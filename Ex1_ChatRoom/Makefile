.PHONY: compile compiles compilec runs runc

compile: server.c client.c config.c config.h
	gcc server.c config.c -o server -lwsock32
	gcc client.c config.c -o client -lwsock32

compiles: server.c config.c config.h
	gcc server.c config.c -o server -lwsock32

compilec: client.c server.c config.c config.h
	gcc client.c config.c -o client -lwsock32

runs: server.exe
	cls
	server.exe 90

runc: client.exe
	cls
	client.exe 10.136.93.245 90