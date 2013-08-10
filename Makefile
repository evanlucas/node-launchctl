DIR=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
REPORTER="spec"
PATH := ${PATH}:/usr/local/bin
NODE_PATH=:/usr/local/lib/node_modules
WGET=$(shell command -v wget)
CURL=$(shell command -v curl)

all: clean deps mac

deps:
	rm -rf $(DIR)/deps/liblaunchctl
	mkdir -p $(DIR)/deps
	curl -sL https://github.com/evanlucas/liblaunchctl/archive/master.zip -o master.zip
	unzip master.zip -d $(DIR)/deps
	mv $(DIR)/deps/liblaunchctl-master $(DIR)/deps/liblaunchctl
	rm -rf ./master.zip

mac:
	./node_modules/.bin/node-gyp configure
	./node_modules/.bin/node-gyp build

test:
	mocha --require should --reporter $(REPORTER) $(DIR)test.js

clean:
	./node_modules/.bin/node-gyp clean
	rm -rf launchctl.node
	rm -rf $(DIR)/deps/liblaunchctl

docs:
	doxx --template template.jade --source ./lib --target docs
  
.PHONY: all docs deps
