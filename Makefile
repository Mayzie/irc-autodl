CC=gcc
DEBUG=-g -DEBUG
RELEASE=-O3
CFLAGS=-Wall -Wextra $(DEBUG)

DEPS = irc-autodl.h client.h
OBJ = irc-autodl.o client.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

all: $(OBJ)
	mkdir -p ./bin/
	$(CC) -o ./bin/irc-autodl $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f ./*.o
