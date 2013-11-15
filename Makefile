
#
# Makefile for lab 6, part 1
#

CC  = gcc
CXX = g++

INCLUDES = -I../../lab3/part1
CFLAGS   = -g -Wall $(INCLUDES)
CXXFLAGS = -g -Wall $(INCLUDES)

LDFLAGS = -g -L../../lab3/part1
LDLIBS = -lmylist

http-server: http-server.o handle-http-client.o

.PHONY: clean
clean:
	rm -f *.o *~ a.out core mdb-lookup-server

.PHONY: all
all: clean default

