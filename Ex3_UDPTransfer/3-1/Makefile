.PHONY: compile compiles compilec runs runc

compile: Server.cpp Client.cpp MyTimer.h Config.cpp Config.h
	g++ Server.cpp Config.cpp -o Server -lwsock32
	g++ Client.cpp Config.cpp -o Client -lwsock32

compiles: Server.cpp MyTimer.h Config.cpp Config.h
	g++ Server.cpp Config.cpp -o Server -lwsock32

compilec: Client.cpp MyTimer.h Config.cpp Config.h
	g++ Client.cpp Config.cpp -o Client -lwsock32

runs: Server.exe
	cls
	Server.exe

runc: Client.exe
	cls
	Client.exe