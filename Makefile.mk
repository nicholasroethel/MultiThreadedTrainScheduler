.phony all:
all: mts

mts: mts.c
	gcc mts.c -pthread -o mts -g 

.PHONY clean:
clean:
	-rm -rf *.o *.exe

