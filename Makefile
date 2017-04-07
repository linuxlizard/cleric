CC=gcc
CFLAGS+=-g -Wall -pedantic -std=c99
LDFLAGS+=-g

all:cleric
cleric:cleric.o

clean:
	$(RM) cleric.o cleric

