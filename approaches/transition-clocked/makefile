# Simple Makefile for RGB SIMPLE COMM program

all: rgb-simple-comm.c
	gcc -g -Wall -o rgb-simple-comm rgb-simple-comm.c

clean:
	$(RM) rgb-simple-comm
	$(RM) ./rgb-simple-comm_output.txt

test: all
	./rgb-simple-comm > ./rgb-simple-comm_output.txt
