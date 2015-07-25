CC=gcc
DEBUG=-g -DEBUG
RELEASE=-O3
CFLAGS=-Wall -Wextra

DEPS = irc-autodl.h client.h
OBJ = irc-autodl.o client.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 

all: $(OBJ)
	mkdir -p ./bin/debug/
	$(CC) -o ./bin/debug/irc-autodl $^ $(CFLAGS) $(DEBUG)

release: $(OBJ)
	mkdir -p ./bin/release/
	$(CC) -o ./bin/release/irc-autodl $^ $(CFLAGS) $(RELEASE)

.PHONY: clean

clean:
	rm -f ./*.o
