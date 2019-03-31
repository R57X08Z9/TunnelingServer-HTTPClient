all: client server

server: server.c
	gcc -o server server.c -lfcgi -lpthread -lcares -I/usr/local/include/json-c/ -ljson-c

client: dns_query_lib.o client.o
	gcc -o client client.o dns_query_lib.o -ljson-c -lcurl

client.o: client.c
	gcc -c client.c

dns_query_lib.o: dns_query_lib.c dns_query_lib.h
	gcc -c dns_query_lib.c -I/usr/local/include/json-c/

libdns_query.so: dns_query_lib.o
	gcc -shared -o libdns_query.so dns_query_lib.o