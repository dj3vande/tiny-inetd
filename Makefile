CC=gcc
CFLAGS=-W -Wall -Wno-missing-field-initializers -ansi -pedantic -O

all: tiny-inetd

clean:
	$(RM) tiny-inetd
