all: server.c client.c
	gcc -o server server.c -lfcgi -lpthread -lcares -I/usr/local/include/json-c/ -ljson-c
	gcc -o client client.c -I/usr/local/include/json-c/ -ljson-c -lcurl
