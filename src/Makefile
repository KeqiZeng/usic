# Makefile

cc = gcc

uname := $(shell uname)

ifeq ($(uname), Darwin)
	cc = clang
endif

objs = main.o server.o client.o utils.o miniaudio.o

usic : $(objs)
	cc -Wall -o usic $(objs)

main.o : server.h client.h utils.h
server.o : server.h config.h utils.h
client.o : client.h config.h utils.h
utils.o : utils.h config.h
miniaudio.o : miniaudio.h

.PHONY : clean
clean :
	rm usic $(objs)
