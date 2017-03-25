CC=gcc
CFLAGS+=-g -Wall -pedantic
LDFLAGS+=-g

all:cleric
cleric:cleric.o

clean:
	$(RM) cleric.o cleric

