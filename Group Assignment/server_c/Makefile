CC=gcc
CFLAGS=-O2 -Wall
LIBS=-lmysqlclient -lpthread

all: server

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LIBS)

clean:
	rm -f server
