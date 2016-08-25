# GCC warning flags are also accepted by clang; users of other compilers
# are assumed to know how to read and edit makefiles.
CFLAGS=-W -Wall -Wno-missing-field-initializers -ansi -pedantic -O

all: tiny-inetd

clean:
	$(RM) tiny-inetd
