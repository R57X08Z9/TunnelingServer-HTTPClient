all: client server

server: server.c
	gcc -o server server.c -lfcgi -lpthread -lcares -I/usr/local/include/json-c/ -ljson-c

client: modul.o client.o
	gcc -o client client.o modul.o -ljson-c -lcurl

client.o: client.c
	gcc -c client.c

modul.o: modul.c modul.h
	gcc -c modul.c -I/usr/local/include/json-c/

libmodul.so: modul.o
	gcc -shared -o libmodul.so modul.o