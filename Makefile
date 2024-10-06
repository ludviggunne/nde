CC=gcc
CFLAGS?=
CFLAGS+=-Wall -Wextra -Wpedantic
LDFLAGS=

OUT=nde
SRC=$(wildcard *.c)
OBJ=$(SRC:%.c=%.o)

PREFIX?=.
BINDIR=$(PREFIX)/bin

$(OUT): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

install:
	install -Dm755 $(OUT) $(BINDIR)/$(OUT)

clean:
	rm -rf $(OUT) $(OBJ) *.fifo

.PHONY: clean install
