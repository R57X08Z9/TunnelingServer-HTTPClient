all: main.c
	gcc -o main main.c -lfcgi -lpthread -lcares -I/usr/local/include/json-c/ -ljson-c