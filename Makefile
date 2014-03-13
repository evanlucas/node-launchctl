DIR=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
REPORTER="spec"
PATH := ${PATH}:/usr/local/bin
NODE_PATH=:/usr/local/lib/node_modules

all: clean travis mac

travis:
	./tests/travis.sh

dev:
	./node_modules/.bin/node-gyp clean
	rm -rf launchctl.node
	make mac

mac:
	./node_modules/.bin/node-gyp configure
	./node_modules/.bin/node-gyp build

clean:
	./node_modules/.bin/node-gyp clean
	rm -rf launchctl.node

.PHONY: all docs dev
