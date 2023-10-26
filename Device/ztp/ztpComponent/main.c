#include "legato.h"
#include "interfaces.h"
#include "le_secStore_common.h"

#define APP_RUNNING_DURATION_SEC 180        //run this app for 3min
#define ENDPOINT_SET_RES "/ztp/endpoint"
#define CERT_SET_RES1 "/ztp/cert1"
#define CERT_SET_RES2 "/ztp/cert2"
#define CERT_SET_RES3 "/ztp/cert3"
#define CERT_SET_RES4 "/ztp/cert4"
#define CERT_SET_RES5 "/ztp/cert5"
#define CERT_SET_RES6 "/ztp/cert6"
#define DEVICE_CONFIG_SET_RES "/deviceConfig"
#define MAX_RESOURCES 20
#define SECRET_ITEM "aws private key"

// reference timer for app session
le_timer_Ref_t sessionTimer;
//reference to AVC event handler
le_avdata_SessionStateHandlerRef_t  avcEventHandlerRef = NULL;
//reference to AVC Session handler
le_avdata_RequestSessionObjRef_t sessionRef = NULL;
//reference to push asset data timer
le_timer_Ref_t serverUpdateTimerRef = NULL;

static char EndpointSet[255];
static char CertSet1[251];
static char CertSet2[251];
static char CertSet3[251];
static char CertSet4[251];
static char CertSet5[251];
static char CertSet6[251];

//-------------------------------------------------------------------------------------------------
/**
 * Device Config setting data handler.
 * This function is returned whenever AirVantage performs a write on /deviceConfig
 */
//-------------------------------------------------------------------------------------------------
static void DeviceConfigHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    int newValue;

    // User has to append the app name to asset data path while writing from server side.
    // Hence use global name space for accessing the value written to this path.
    le_avdata_SetNamespace(LE_AVDATA_NAMESPACE_GLOBAL);
    le_result_t resultGetInt = le_avdata_GetInt(path, &newValue);
    le_avdata_SetNamespace(LE_AVDATA_NAMESPACE_APPLICATION);

    if (LE_OK == resultGetInt)
    {
        LE_INFO("%s set to %d", path, newValue);
    }
    else
    {
        LE_ERROR("Error in getting setting %s - Error = %d", path, resultGetInt);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Setting data handler.
 * This function is returned whenever AirVantage performs a read or write on the Endpoint
 */
//-------------------------------------------------------------------------------------------------
static void EndpointSettingHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    LE_INFO("------------------- Server writing ENDPOINT ------------");

    le_result_t resultGetEndpoint = le_avdata_GetString(ENDPOINT_SET_RES, EndpointSet, sizeof(EndpointSet));
    if (LE_FAULT == resultGetEndpoint) 
    {
        LE_ERROR("Error in getting latest ENDPOINT");
    } else {
        LE_INFO("ENDPOINT from server is ==> %s", EndpointSet);
        FILE * fd = fopen("/home/root/ztp/aws_iot_endpoint.txt", "w");
        if (fd == NULL) {
		    LE_ERROR("Endpoint file could not be created");
            exit(EXIT_FAILURE);
        } 
        fwrite (EndpointSet, sizeof(char), strlen(EndpointSet), fd);
        fclose (fd);
        LE_INFO("ENDPOINT file written!");        
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Setting data handler.
 * This function is returned whenever AirVantage performs a read or write on the Cert
 */
//-------------------------------------------------------------------------------------------------
static void CertSettingHandler
(
    const char* path,
    le_avdata_AccessType_t accessType,
    le_avdata_ArgumentListRef_t argumentList,
    void* contextPtr
)
{
    LE_INFO("------------------- Server writing CERT ------------");

    // TODO: read size from AV and do malloc based on that?
    // CertSet = (char *) malloc(1420);
    
    le_result_t resultGetCert1 = le_avdata_GetString(CERT_SET_RES1, CertSet1, sizeof(CertSet1));
    le_result_t resultGetCert2 = le_avdata_GetString(CERT_SET_RES2, CertSet2, sizeof(CertSet2));
    le_result_t resultGetCert3 = le_avdata_GetString(CERT_SET_RES3, CertSet3, sizeof(CertSet3));
    le_result_t resultGetCert4 = le_avdata_GetString(CERT_SET_RES4, CertSet4, sizeof(CertSet4));
    le_result_t resultGetCert5 = le_avdata_GetString(CERT_SET_RES5, CertSet5, sizeof(CertSet5));
    le_result_t resultGetCert6 = le_avdata_GetString(CERT_SET_RES6, CertSet6, sizeof(CertSet6));
    if ((LE_FAULT == resultGetCert1) || 
        (LE_FAULT == resultGetCert2) || 
        (LE_FAULT == resultGetCert3) || 
        (LE_FAULT == resultGetCert4) || 
        (LE_FAULT == resultGetCert5) || 
        (LE_FAULT == resultGetCert6)) 
    {
        LE_ERROR("Error in getting latest CERT");
    } else {
        LE_INFO("CERT1 from server is ==> %s", CertSet1);
        LE_INFO("CERT2 from server is ==> %s", CertSet2);
        LE_INFO("CERT3 from server is ==> %s", CertSet3);
        LE_INFO("CERT4 from server is ==> %s", CertSet4);
        LE_INFO("CERT5 from server is ==> %s", CertSet5);
        LE_INFO("CERT6 from server is ==> %s", CertSet6);
        FILE * fd = fopen("/home/root/ztp/aws_iot_cert.pem.crt", "w");
        if (fd == NULL) {
		    LE_ERROR("Certificate file could not be created");
            exit(EXIT_FAILURE);
        } 
        fwrite (CertSet1, sizeof(char), strlen(CertSet1), fd);
        fwrite (CertSet2, sizeof(char), strlen(CertSet2), fd);
        fwrite (CertSet3, sizeof(char), strlen(CertSet3), fd);
        fwrite (CertSet4, sizeof(char), strlen(CertSet4), fd);
        fwrite (CertSet5, sizeof(char), strlen(CertSet5), fd);
        fwrite (CertSet6, sizeof(char), strlen(CertSet6), fd);
        fclose (fd);
        LE_INFO("CERT file written!");        
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Push ack callback handler
 * This function is called whenever push has been performed successfully in AirVantage server
 */
//-------------------------------------------------------------------------------------------------
static void PushCallbackHandler
(
    le_avdata_PushStatus_t status,
    void* contextPtr
)
{
    switch (status)
    {
        case LE_AVDATA_PUSH_SUCCESS:
            LE_INFO("Legato ZTP push successfully");
            break;
        case LE_AVDATA_PUSH_FAILED:
            LE_INFO("Legato ZTP push failed");
            break;
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Push ack callback handler
 * This function is called every 10 seconds to push the data and update data in AirVantage server
 */
//-------------------------------------------------------------------------------------------------
void PushResources(le_timer_Ref_t  timerRef)
{
    // if session is still open, push the values
    if (NULL != avcEventHandlerRef)
    {
        int fd = open("/home/root/ztp/aws_iot_csr.csr", O_RDONLY);
        if (fd < 0) {
		    LE_ERROR("CSR file could not be accessed");
            exit(EXIT_FAILURE);
        }
        le_result_t r = le_avdata_PushStream("CSR_STREAM", fd, PushCallbackHandler, NULL);
        if (r != LE_OK)
        {
            LE_ERROR("Failed to push CSR over stream - fd=%d - %s", fd, LE_RESULT_TXT(r));
            exit(EXIT_FAILURE);
        }
        else
        {
            LE_INFO("CSR succesfully pushed over stream - fd=%d", fd);
        }        
        close(fd);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Function relevant to AirVantage server connection
 */
//-------------------------------------------------------------------------------------------------
static void sig_appTermination_cbh(int sigNum)
{
    LE_INFO("Close AVC session");
    le_avdata_ReleaseSession(sessionRef);
    if (NULL != avcEventHandlerRef)
    {
        //unregister the handler
        LE_INFO("Unregister the session handler");
        le_avdata_RemoveSessionStateHandler(avcEventHandlerRef);
    }
}

//-------------------------------------------------------------------------------------------------
/**
 * Status handler for avcService updates
 */
//-------------------------------------------------------------------------------------------------
static void avcStatusHandler
(
    le_avdata_SessionState_t updateStatus,
    void* contextPtr
)
{
    switch (updateStatus)
    {
        case LE_AVDATA_SESSION_STARTED:
            LE_INFO("Legato session started successfully");
            break;
        case LE_AVDATA_SESSION_STOPPED:
            LE_INFO("Legato session stopped");
            break;
    }
}

static void timerExpiredHandler(le_timer_Ref_t  timerRef)
{
    sig_appTermination_cbh(0);

    LE_INFO("Legato ZtpApp Ended");

    //Quit the app
    exit(EXIT_SUCCESS);
}

static void createPrivateKeyAndCSR()
{
    char privateKeyLine[66];
    char privateKey[2048];

    // generate private key
    FILE *fpk = popen("/usr/bin/openssl genrsa 2048", "r");
    if (fpk == NULL) {
        LE_ERROR("Failed to run openssl command to generate key\n" );
        exit(EXIT_FAILURE);
    }

    // read the output a line at a time
    while (fgets(privateKeyLine, sizeof(privateKeyLine), fpk) != NULL) {
        // printf("%s", privateKeyLine);
        strcat(privateKey, privateKeyLine);
    }

    // close
    pclose(fpk);
    
    LE_INFO("Private key created!");

    // generate CSR
    FILE *fcsr = popen("/usr/bin/openssl req -new -key - -out /home/root/ztp/aws_iot_csr.csr -subj \"/C=ES/ST=Madrid/L=Madrid/O=Semtech, Inc./OU=IT/CN=ztp.semtech.com\"", "w");
    if (fcsr == NULL) {
        LE_ERROR("Failed to run openssl command to generate CSR\n" );
        exit(EXIT_FAILURE);
    }
    
    // Inject private key to stdin
    const size_t privateKeyLen = strlen(privateKey);
    size_t nNumWritten = fwrite(privateKey, 1, privateKeyLen, fcsr);
    if (nNumWritten != privateKeyLen) {
        LE_ERROR("Failed to inject private key via stdin to openssl to generate CSR\n" );
        exit(EXIT_FAILURE);
    }
    
    // close
    pclose(fcsr);    

    LE_INFO("CSR created!");
    
    // Write the SECRET_ITEM.
    le_result_t result = le_secStore_Write(SECRET_ITEM, (uint8_t*)privateKey, sizeof(privateKey));
    if (result == LE_OK) { 
        LE_INFO("Succesfully write secret to sec store.  %s.", LE_RESULT_TXT(result));
    } else {
        LE_ERROR("Failed to write secret to sec store\n" );
        exit(EXIT_FAILURE);
    }
}

COMPONENT_INIT
{
    LE_INFO("Start Legato ZtpApp");

    le_sig_Block(SIGTERM);
    le_sig_SetEventHandler(SIGTERM, sig_appTermination_cbh);
    
    // create /home/root/ztp folder
    struct stat st = {0};
    if (stat("/home/root/ztp", &st) == -1) {
        mkdir("/home/root/ztp", 0700);
    }    

    createPrivateKeyAndCSR();

    //Start AVC Session
    //Register AVC handler
    avcEventHandlerRef = le_avdata_AddSessionStateHandler(avcStatusHandler, NULL);
    //Request AVC session. Note: AVC handler must be registered prior starting a session
    le_avdata_RequestSessionObjRef_t sessionRequestRef = le_avdata_RequestSession();
    if (NULL == sessionRequestRef)
    {
        LE_ERROR("AirVantage Connection Controller does not start.");
    } else {
        sessionRef=sessionRequestRef;
        LE_INFO("AirVantage Connection Controller started.");
    }

    // [CreateTimer]
    LE_INFO("Started LWM2M session with AirVantage");
    sessionTimer = le_timer_Create("ZtpAppSessionTimer");
    le_clk_Time_t avcInterval = {APP_RUNNING_DURATION_SEC, 0};
    le_timer_SetInterval(sessionTimer, avcInterval);
    le_timer_SetRepeat(sessionTimer, 1);
    le_timer_SetHandler(sessionTimer, timerExpiredHandler);
    le_timer_Start(sessionTimer);

    // [CreateResources]
    LE_INFO("Create instances ztp ");
    le_result_t resultCreateEndpoint = le_avdata_CreateResource(ENDPOINT_SET_RES, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateEndpoint)
    {
        LE_ERROR("Error in creating ENDPOINT_SET_RES");
    }
    le_result_t resultCreateCert1 = le_avdata_CreateResource(CERT_SET_RES1, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateCert1)
    {
        LE_ERROR("Error in creating CERT_SET_RES1");
    }
    le_result_t resultCreateCert2 = le_avdata_CreateResource(CERT_SET_RES2, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateCert2)
    {
        LE_ERROR("Error in creating CERT_SET_RES2");
    }
    le_result_t resultCreateCert3 = le_avdata_CreateResource(CERT_SET_RES3, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateCert3)
    {
        LE_ERROR("Error in creating CERT_SET_RES3");
    }
    le_result_t resultCreateCert4 = le_avdata_CreateResource(CERT_SET_RES4, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateCert4)
    {
        LE_ERROR("Error in creating CERT_SET_RES4");
    }
    le_result_t resultCreateCert5 = le_avdata_CreateResource(CERT_SET_RES5, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateCert5)
    {
        LE_ERROR("Error in creating CERT_SET_RES5");
    }
    le_result_t resultCreateCert6 = le_avdata_CreateResource(CERT_SET_RES6, LE_AVDATA_ACCESS_SETTING);
    if (LE_FAULT == resultCreateCert6)
    {
        LE_ERROR("Error in creating CERT_SET_RES6");
    }
    
    // [RegisterHandler]
    //Register handler for Variables, Settings and Commands
    LE_INFO("Register handler of paths");
    le_avdata_AddResourceEventHandler(ENDPOINT_SET_RES, EndpointSettingHandler, NULL);
    le_avdata_AddResourceEventHandler(CERT_SET_RES1, CertSettingHandler, NULL);
    le_avdata_AddResourceEventHandler(CERT_SET_RES2, CertSettingHandler, NULL);
    le_avdata_AddResourceEventHandler(CERT_SET_RES3, CertSettingHandler, NULL);
    le_avdata_AddResourceEventHandler(CERT_SET_RES4, CertSettingHandler, NULL);
    le_avdata_AddResourceEventHandler(CERT_SET_RES5, CertSettingHandler, NULL);
    le_avdata_AddResourceEventHandler(CERT_SET_RES6, CertSettingHandler, NULL);

    // [PushTimer]
    //Set timer to update on server on a regular basis
    serverUpdateTimerRef = le_timer_Create("serverUpdateTimer");
    le_clk_Time_t serverUpdateInterval = { 60, 0 }; //update server every 60 seconds
    le_timer_SetInterval(serverUpdateTimerRef, serverUpdateInterval);
    le_timer_SetRepeat(serverUpdateTimerRef, 1); //set repeat to 1 (initial send CSR)
    le_timer_SetHandler(serverUpdateTimerRef, PushResources); //set callback function to handle timer expiration event
    le_timer_Start(serverUpdateTimerRef); //start timer

    le_result_t resultDeviceConfig;
    char path[LE_AVDATA_PATH_NAME_BYTES];
    int i;

    // Create device config resources
    for (i = 0; i < MAX_RESOURCES; i++)
    {
        snprintf(path, sizeof(path), "%s/%d", DEVICE_CONFIG_SET_RES, i);

        LE_INFO("Creating asset %s", path);

        resultDeviceConfig = le_avdata_CreateResource(path, LE_AVDATA_ACCESS_SETTING);
        if (LE_FAULT == resultDeviceConfig)
        {
           LE_ERROR("Error in creating DEVICE_CONFIG_SET_RES");
        }
    }

    // Add resource handler at the device config (root)
    LE_INFO("Add resource event handler");
    le_avdata_AddResourceEventHandler(DEVICE_CONFIG_SET_RES, DeviceConfigHandler, NULL);
}