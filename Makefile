CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -lrt -lpthread

.PHONY: all clean zip

all: supervisor generator

generator: generator.o fb_arc_set.o
	$(CC) $(LDFLAGS) -o $@ $^

supervisor: supervisor.o fb_arc_set.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

supervisor.o: supervisor.c fb_arc_set.h structs.h
generator.o: generator.c fb_arc_set.h structs.h
fb_arc_set.o: fb_arc_set.c fb_arc_set.h structs.h

clean:
	rm -rf generator supervisor *.o *.tgz

zip:
	tar -cvzf fb_arc_set_1426981.tgz generator.c supervisor.c fb_arc_set.c fb_arc_set.h structs.h Makefile
