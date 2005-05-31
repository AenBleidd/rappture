#
# Simple makefile for building EasyMesh
#
CC = gcc
CFLAGS = -O3
LIBS = -L/usr/X11R6/lib -lX11 -lm

all: easymesh showmesh

easymesh: easymesh_1_4.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

showmesh: showmesh_1_0.c
	$(CC) $(CFLAGS) $< -o $@ $(LIBS)

clean:
	rm -f easymesh showmesh
