all:
	make clean mac

mac:
	CXX=g++ node-gyp configure
	CXX=g++ node-gyp build
	ln -s build/Release/launchctl.node launchctl.node

clean:
		node-gyp clean
		rm launchctl.node

.PHONY: clean mac
