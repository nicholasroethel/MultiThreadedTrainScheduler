.phony all:
all: mts

mts: mts.c
	gcc mts.c -o mts -pthread -g

.PHONY clean:
clean:
	-rm -rf *.o *.exe

