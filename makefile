all: server.c
	gcc -o server server.c -lfcgi -lpthread -lcares -I/usr/local/include/json-c/ -ljson-c
