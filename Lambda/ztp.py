import base64
import json
import boto3
import http.client
import os
import urllib.parse

print('Loading function')

client = boto3.client('iot')

av_username = os.environ['username']
av_password = urllib.parse.quote(os.environ['password'])
av_client_id = os.environ['client_id']
av_client_secret = os.environ['client_secret']

def lambda_handler(event, context):
    #print("Received event: " + json.dumps(event, indent=2))
    for record in event['Records']:
        # Kinesis data is base64 encoded so decode here
        payload = base64.b64decode(record['kinesis']['data']).decode('utf-8')
        print("Decoded payload: " + payload)
        data = json.loads(payload)
        if data.get('content'):
            content = data.get('content')
            if content.get('CSR_STREAM'):
                # describe endpoint
                response = client.describe_endpoint(endpointType='iot:Data-ATS')
                endpoint_address = response['endpointAddress']
                print("describe_endpoint: " + json.dumps(response, indent=2))
              
                # certificate creation
                csr = content.get('CSR_STREAM')
                print("CSR: " + csr)
                response = client.create_certificate_from_csr(certificateSigningRequest=csr, setAsActive=True)
                certificate_pem = response['certificatePem']
                print("create_certificate_from_csr: " + json.dumps(response, indent=2))
                
                # policy attachment
                certificate_arn = response['certificateArn']
                print("Cert ARN: " + certificate_arn)
                response = client.attach_policy(policyName="ztp", target=certificate_arn)
                print("attach_policy: " + json.dumps(response, indent=2))
                
                # request to get AV token
                conn = http.client.HTTPSConnection("eu.airvantage.net")
                token_payload = 'grant_type=password&username=' + av_username + '&password=' + av_password + '&client_id=' + av_client_id + '&client_secret=' + av_client_secret
                token_headers = {
                  'Content-Type': 'application/x-www-form-urlencoded'
                }
                conn.request("POST", "/api/oauth/token", token_payload, token_headers)
                token_res = conn.getresponse()
                token_res_data = token_res.read()
                token = json.loads(token_res_data.decode("utf-8")).get('access_token')
                print("token: " + token)

                # request to AV apply settings
                conn = http.client.HTTPSConnection("eu.airvantage.net")
                apply_settings_payload = json.dumps({
                  "reboot": False,
                  "protocol": "LWM2M",
                  "systems": {
                    "uids": [
                      content.get('system.id')
                    ]
                  },
                  "settings": [
                    {
                      "key": "/ztp/ztp/endpoint",
                      "value": endpoint_address
                    },
                    {
                      "key": "/ztp/ztp/cert1",
                      "value": certificate_pem[:250]
                    },
                    {
                      "key": "/ztp/ztp/cert2",
                      "value": certificate_pem[250:250*2]
                    },
                    {
                      "key": "/ztp/ztp/cert3",
                      "value": certificate_pem[250*2:250*3]
                    },
                    {
                      "key": "/ztp/ztp/cert4",
                      "value": certificate_pem[250*3:250*4]
                    },
                    {
                      "key": "/ztp/ztp/cert5",
                      "value": certificate_pem[250*4:250*5]
                    },
                    {
                      "key": "/ztp/ztp/cert6",
                      "value": certificate_pem[250*5:]
                    }                    
                  ]
                })
                apply_settings_headers = {
                  'Content-Type': 'application/json',
                  'Authorization': 'Bearer ' + token
                }
                conn.request("POST", "/api/v1/operations/systems/settings", apply_settings_payload, apply_settings_headers)
                apply_settings_res = conn.getresponse()
                apply_settings_data = apply_settings_res.read()
                print("Apply settings response: " + apply_settings_data.decode("utf-8"))
    return 'Successfully processed {} records.'.format(len(event['Records']))
