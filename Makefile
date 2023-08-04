# Makefile

CC = gcc

UNAME := $(shell uname)

ifeq ($(UNAME), Darwin)
	CC = clang
endif

SRC_DIR := ./src
# objs = main.o server.o client.o utils.o miniaudio.o
SOURCES := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(SOURCES:.c=.o)
INCLUDES := -I$(SRC_DIR)

usic : $(OBJS)
	cc -Wall -Wextra -o usic $(OBJS)

main.o : server.h client.h utils.h
server.o : server.h config.h utils.h
client.o : client.h config.h utils.h
utils.o : utils.h config.h
miniaudio.o : miniaudio.h

.PHONY : clean install
clean :
	rm -f usic $(OBJS)

install : usic
	install -m 755 usic /usr/local/bin
