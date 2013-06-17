DIR=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
REPORTER="spec"
PATH := ${PATH}:/usr/local/bin
NODE_PATH=:/usr/local/lib/node_modules
OS_VERS=$(shell sw_vers -productVersion)
SDK_PATH=$(/usr/bin/xcrun --show-sdk-path)

all:
	make clean mac

mac:
	rm -rf $(DIR)/deps/liblaunchctl
	mkdir -p $(DIR)/deps
	wget https://github.com/evanlucas/liblaunchctl/archive/master.zip -O master.zip
	unzip master.zip -d $(DIR)/deps
	mv $(DIR)/deps/liblaunchctl-master $(DIR)/deps/liblaunchctl
	rm -rf ./master.zip
	/usr/local/bin/node-gyp configure
	/usr/local/bin/node-gyp build

test:
	mocha --require should --reporter $(REPORTER) --growl $(DIR)test.js

clean:
	@echo ${SDK_PATH}
	/usr/local/bin/node-gyp clean
	rm -rf launchctl.node
	rm -rf $(DIR)/deps/liblaunchctl

show:
	@echo $(xcrun --show-sdk-path)

.PHONY: all
