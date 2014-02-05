# makefile for macosx

flags = -Wall -DDEBUG # -O3

net_headers := $(wildcard *.h)
lan_headers := $(wildcard lan/*.h)

all : Client Server Test

NetTransport.o : makefile NetTransport.h NetTransport.cpp NetPlatform.h ${lan_headers}
	g++ NetTransport.cpp -c -o NetTransport.o ${flags}

libtransport.a : NetTransport.o
	ar rcs libtransport.a NetTransport.o

% : %.cpp libtransport.a #{net_headers}
	g++ $< -o $@ -L. -ltransport ${flags}

transport : libtransport.a
	
client : Client
	./Client
	
server : Server
	./Server

lobby : Lobby
	./Lobby

test : Test
	./Test
	
clean:
	rm -f Client Server Lobby Test *.o *.a
