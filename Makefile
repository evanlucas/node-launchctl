DIR=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
REPORTER="spec"

all:
	make clean mac

mac:
	rm -rf $(DIR)/deps/liblaunchctl
	mkdir -p $(DIR)/deps
	wget https://github.com/evanlucas/liblaunchctl/archive/master.zip -O master.zip
	unzip master.zip -d $(DIR)/deps
	mv $(DIR)/deps/liblaunchctl-master $(DIR)/deps/liblaunchctl
	rm -rf ./master.zip
	node-gyp configure
	node-gyp build

test:
	mocha --require should --reporter $(REPORTER) --growl

clean:
	node-gyp clean
	rm -rf launchctl.node
	rm -rf $(DIR)/deps/liblaunchctl

.PHONY: mac
