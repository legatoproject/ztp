TARGETS := $(MAKECMDGOALS)

.PHONY: all $(TARGETS)
all: $(TARGETS)

$(TARGETS):
	mkapp -v -t $@ \
		-i $(LEGATO_ROOT)/interfaces/modemServices \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/include \
	    -i /home/ubuntu/myWorkspace/apps/awsMqtt/platform/linux/common \
	    -i /home/ubuntu/myWorkspace/apps/awsMqtt/platform/linux/mbedtls \
	    -i /home/ubuntu/myWorkspace/apps/awsMqtt/external_libs/mbedTLS/include \
	    -i /home/ubuntu/myWorkspace/apps/awsMqtt/external_libs/jsmn \
		-i /home/ubuntu/myWorkspace/apps/awsMqtt/awsMqttComponent \
	    -C -Wno-error=format-truncation \
		awsMqtt.adef

clean:
	rm -rf _build_* *.update
