CC  = gcc
CXX = g++

INCLUDES = 
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = -g 
LDLIBS = 

http-server: http-server.o handle-http-client.o

.PHONY: clean
clean:
	rm -f *.o *~ a.out core mdb-lookup-server

.PHONY: all
all: clean default

