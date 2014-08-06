

CC=$(CROSS_COMPILE)gcc

CFLAGS=-Wall

all: m4boot

m4boot: m4boot.o
	$(CC) $(CFLAGS) -o m4boot m4boot.o

.PHONY : clean
clean:
	-rm m4boot m4boot.o
