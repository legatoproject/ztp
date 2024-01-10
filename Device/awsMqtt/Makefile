TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		-i $(LEGATO_ROOT)/interfaces/modemServices \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/mqttDemoMutualAuth \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/libraries/3rdparty/mbedtls/configs \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/libraries/3rdparty/mbedtls/include \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/libraries/3rdparty/mbedtls/include/psa \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/libraries/3rdparty/mbedtls/include/mbedtls \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/libraries/3rdparty/mbedtls/library \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/libraries/standard/coreMQTT/source/include \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/libraries/standard/coreMQTT/source/interface \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/libraries/standard/backoffAlgorithm/source/include \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/demos/logging-stack \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/demos/include \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/platform/posix/include \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/platform/posix/transport/include \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/aws-iot-device-sdk-embedded-C/platform/include \
	    -C -Wno-error=format-truncation \
	    -C -Wno-error=return-type \
	    -C -Wno-error=format \
	    -C -Wno-error=pointer-sign \
		awsMqtt.adef

clean:
	rm -rf _build_* *.update