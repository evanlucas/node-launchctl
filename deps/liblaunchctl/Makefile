PATH := ${PATH}:/usr/local/bin
NODE_PATH=:/usr/local/lib/node_modules
GYP := $(shell which node-gyp 2>/dev/null)

all:
	scripts/install.sh
	${GYP} clean
	${GYP} configure
	${GYP} build

clean:
	xcodebuild clean
	${GYP} clean

test:
	xcodebuild -scheme make test

.PHONY: all
