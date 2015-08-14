
INCLUDES=-I.
CFLAGS=-std=gnu99 -O -g -Wall -fno-builtin-round -fno-builtin-trunc -MD
LDFLAGS=

all: segedit

segedit: segedit.o bytesex.o
	gcc $(LDFLAGS) -o $@ segedit.o bytesex.o

segedit.o: segedit.c
	gcc -c $(CFLAGS) $(INCLUDES) -o segedit.o segedit.c

bytesex.o: bytesex.c
	gcc -c $(CFLAGS) $(INCLUDES) -o bytesex.o bytesex.c

clean:
	rm -f segedit *.o *.d
