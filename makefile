all: client server

server: server.c
	gcc -o server server.c -lfcgi -lpthread -lcares -I/usr/local/include/json-c/ -ljson-c

client: client.o modul.o
	gcc -o client client.o modul.o -ljson-c -lcurl

client.o: client.c
	gcc -c client.c

modul.o: modul.c modul.h
	gcc -c modul.c -I/usr/local/include/json-c/