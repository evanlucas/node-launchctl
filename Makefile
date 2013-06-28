DIR=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
REPORTER="spec"
PATH := ${PATH}:/usr/local/bin
NODE_PATH=:/usr/local/lib/node_modules

all: clean mac

deps:
	rm -rf $(DIR)/deps/liblaunchctl
	mkdir -p $(DIR)/deps
	wget https://github.com/evanlucas/liblaunchctl/archive/master.zip -O master.zip
	unzip master.zip -d $(DIR)/deps
	mv $(DIR)/deps/liblaunchctl-master $(DIR)/deps/liblaunchctl
	rm -rf ./master.zip

mac:
	/usr/local/bin/node-gyp configure
	/usr/local/bin/node-gyp build

test:
	mocha --require should --reporter $(REPORTER) --growl $(DIR)test.js

clean:
	/usr/local/bin/node-gyp clean
	rm -rf launchctl.node
	rm -rf $(DIR)/deps/liblaunchctl
	mkdir -p $(DIR)/deps
	cp -r ../liblaunchctl $(DIR)/deps/

docs:
	doxx --template template.jade --source ./lib --target docs
  
.PHONY: all docs deps
