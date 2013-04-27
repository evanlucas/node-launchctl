REPORTER="spec"

all:
	make clean mac

mac:
	CXX=g++ node-gyp configure
	CXX=g++ node-gyp build
	ln -s build/Release/launchctl.node launchctl.node

test:
	mocha --reporter $(REPORTER)

clean:
		node-gyp clean
		rm -rf launchctl.node

.PHONY: clean mac
