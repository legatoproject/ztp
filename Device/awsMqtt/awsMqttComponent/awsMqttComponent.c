
// sdkComponent.c
//
// Specifies the main source file of the component. Add initialization and event registrations to
// the files COMPONENT_INIT functions.

// Include the core framework C APIs.
#include "legato.h"
#include <unistd.h>

// Include your component's API interfaces.
#include "interfaces.h"
#include "le_secStore_common.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"

#define TIMEOUT_SECS 30
#define SECRET_ITEM "aws private key"
#define MSG_PAYLOAD_MAX 50
/**
 * @brief Default cert location
 */
static char certDirectory[PATH_MAX + 1] = "/home/root/ztp";

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
static uint32_t port = AWS_IOT_MQTT_PORT;

/**
 * @brief This parameter will avoid infinite loop of publish and exit the program after certain number of publishes
 */
static uint32_t publishCount = 10;

static le_data_RequestObjRef_t ConnectionRef;
static bool WaitingForConnection;

static void TimeoutHandler
(
    le_timer_Ref_t timerRef
)
{
    if (WaitingForConnection)
    {
        LE_ERROR("Couldn't establish connection after " STRINGIZE(TIMEOUT_SECS) " seconds");
        exit(EXIT_FAILURE);
    }
}

static void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data)
{
	LE_ERROR("MQTT Disconnect");
	IoT_Error_t rc = FAILURE;

	if (NULL == pClient)
	{
		return;
	}

	LE_INFO(data);

	if (aws_iot_is_autoreconnect_enabled(pClient))
	{
		LE_INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
	}
	else
	{
		LE_ERROR("Auto Reconnect not enabled. Starting manual reconnect...");
		rc = aws_iot_mqtt_attempt_reconnect(pClient);
		if (NETWORK_RECONNECTED == rc)
		{
			LE_ERROR("Manual Reconnect Successful");
		}
		else
		{
			LE_ERROR("Manual Reconnect Failed - %d", rc);
		}
	}
}

static int run_main()
{

	bool infinitePublishFlag = true;

	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char cPayload[100];

	int32_t i = 0;

    char *HostAddress;
    size_t HostAddressSize = 255;

	IoT_Error_t rc = FAILURE;

	AWS_IoT_Client client;
	IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
	IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;

	IoT_Publish_Message_Params paramsQOS0;

	LE_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	snprintf(rootCA, PATH_MAX + 1, "%s/%s", certDirectory, AWS_IOT_ROOT_CA_FILENAME);
	snprintf(clientCRT, PATH_MAX + 1, "%s/%s", certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
	snprintf(clientKey, PATH_MAX + 1, "%s/%s", certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);

	// dowload AmazonCA cert
	if (access("/home/root/ztp/AmazonRootCA1.pem", F_OK) != 0) {
		if (system("curl -o /home/root/ztp/AmazonRootCA1.pem https://www.amazontrust.com/repository/AmazonRootCA1.pem") != 0) {
			LE_ERROR("AWS CA cert could not be downloaded");
        	exit(EXIT_FAILURE);
		}
	}

    // read AWS IoT Core endpoint
    FILE * fp = fopen("/home/root/ztp/aws_iot_endpoint.txt", "r");
    if (fp == NULL) {
		LE_ERROR("AWS IoT Core endpoint file not found");
        exit(EXIT_FAILURE);
    }
    HostAddress = (char *)malloc(HostAddressSize * sizeof(char));
    getline(&HostAddress, &HostAddressSize, fp);
 	LE_INFO("AWS IoT Endpoint %s", HostAddress);
    fclose(fp);	

	LE_INFO("rootCA %s", rootCA);
	LE_INFO("clientCRT %s", clientCRT);
	LE_INFO("clientKey %s", clientKey);
	mqttInitParams.enableAutoReconnect = false; // We enable this later below
	mqttInitParams.pHostURL = HostAddress;
	mqttInitParams.port = port;
	mqttInitParams.pRootCALocation = rootCA;
	mqttInitParams.pDeviceCertLocation = clientCRT;
	mqttInitParams.pDevicePrivateKeyLocation = clientKey;
	mqttInitParams.mqttCommandTimeout_ms = 20000;
	mqttInitParams.tlsHandshakeTimeout_ms = 10000;
	mqttInitParams.isSSLHostnameVerify = true;
	mqttInitParams.disconnectHandler = disconnectCallbackHandler;
	mqttInitParams.disconnectHandlerData = NULL;

	// extract private key from secure storage and write it to file
    char privateKeyFromKeyStore[2048];
    size_t privateKeyFromKeyStoreSize = sizeof(privateKeyFromKeyStore);
    privateKeyFromKeyStoreSize = sizeof(privateKeyFromKeyStore);
    le_result_t result = le_secStore_Read(SECRET_ITEM, (uint8_t*)privateKeyFromKeyStore, &privateKeyFromKeyStoreSize);
    LE_INFO("Read secret from sec store.  %s.", LE_RESULT_TXT(result));
    FILE * fd = fopen(clientKey, "w");
    if (fd == NULL) {
		LE_ERROR("Private key temporary file could not be created");
        exit(EXIT_FAILURE);
    }    
    fwrite(privateKeyFromKeyStore, sizeof(char), strlen(privateKeyFromKeyStore), fd);
    fclose(fd);

	rc = aws_iot_mqtt_init(&client, &mqttInitParams);
	if (SUCCESS != rc)
	{
		LE_ERROR("aws_iot_mqtt_init returned error : %d ", rc);
		return rc;
	}

	connectParams.keepAliveIntervalInSec = 600;
	connectParams.isCleanSession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = AWS_IOT_MQTT_CLIENT_ID;
	connectParams.clientIDLen = (uint16_t)strlen(AWS_IOT_MQTT_CLIENT_ID);
	connectParams.isWillMsgPresent = false;

	LE_INFO("Connecting...");
	rc = aws_iot_mqtt_connect(&client, &connectParams);
	if (SUCCESS != rc)
	{
		LE_ERROR("Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
		return rc;
	}
	LE_INFO("Connected...");
	
	// delete private key file (it will remain in SecureStorage)
    if (remove(clientKey) == 0) 
    {    
    	LE_INFO("Secret key file removed successfully");
    }    
    else
    {    
		LE_ERROR("Error removing secret key file");
        exit(EXIT_FAILURE);
    }
    
	/*
	 * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
	 *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
	 *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
	 */
	rc = aws_iot_mqtt_autoreconnect_set_status(&client, true);
	if (SUCCESS != rc)
	{
		LE_ERROR("Unable to set Auto Reconnect to true - %d", rc);
		return rc;
	}

	snprintf(cPayload, MSG_PAYLOAD_MAX + 1, "%s : %d ", "test from ZTP ", i);

	paramsQOS0.qos = QOS0;
	paramsQOS0.payload = (void *)cPayload;
	paramsQOS0.isRetained = 0;

	if (publishCount != 0)
	{
		infinitePublishFlag = false;
	}

	while ((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) && (publishCount > 0 || infinitePublishFlag))
	{

		//Max time the yield function will wait for read messages
		rc = aws_iot_mqtt_yield(&client, 100);
		if (NETWORK_ATTEMPTING_RECONNECT == rc)
		{
			// If the client is attempting to reconnect we will skip the rest of the loop.
			continue;
		}

		LE_INFO("Sleeping and publishing ...");
		sleep(1);
		snprintf(cPayload, MSG_PAYLOAD_MAX + 1, "%s : %d ", "test from ZTP with QOS0 ", i++);
		paramsQOS0.payloadLen = strlen(cPayload);
		rc = aws_iot_mqtt_publish(&client, "ztp/fx30s", strlen("ztp/fx30s"), &paramsQOS0);
		if (publishCount > 0)
		{
			publishCount--;
		}

		if (publishCount == 0 && !infinitePublishFlag)
		{
			break;
		}
	}

	// Wait for all the messages to be received
	aws_iot_mqtt_yield(&client, 100);

	if (SUCCESS != rc)
	{
		LE_ERROR("An error occurred in the loop.\n");
	}
	else
	{
		LE_INFO("Publish done\n");
	}
	
	free(HostAddress);

	return rc;
}

static void ConnectionStateHandler(
	const char *intfName,
	bool isConnected,
	void *contextPtr)
{
	if (isConnected)
	{
		WaitingForConnection = false;
		LE_INFO("Interface %s connected.", intfName);
		run_main();
		le_data_Release(ConnectionRef);
	}
	else
	{
		LE_INFO("Interface %s disconnected.", intfName);
	}
}

COMPONENT_INIT
{
	LE_INFO("Starting AWS SDK");

    le_timer_Ref_t timerPtr = le_timer_Create("Connection timeout timer");
    le_clk_Time_t interval = {TIMEOUT_SECS, 0};
    le_timer_SetInterval(timerPtr, interval);
    le_timer_SetHandler(timerPtr, &TimeoutHandler);
    WaitingForConnection = true;
    le_timer_Start(timerPtr);

    le_data_AddConnectionStateHandler(&ConnectionStateHandler, NULL);
    LE_INFO("Requesting connection...");
    ConnectionRef = le_data_Request();
}