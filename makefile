all: client server

client: client.c
	gcc -o client client.c -I/usr/local/include/json-c/ -ljson-c -lcurl

server: server.c
	gcc -o server server.c -lfcgi -lpthread -lcares -I/usr/local/include/json-c/ -ljson-c