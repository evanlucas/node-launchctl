REPORTER="spec"

all:
	make clean mac

mac:
	CXX=g++ node-gyp configure
	CXX=g++ node-gyp build

test:
	mocha --require should --reporter $(REPORTER)	--growl

clean:
		node-gyp clean
		rm -rf launchctl.node
		rm -rf deps

.PHONY: clean mac
