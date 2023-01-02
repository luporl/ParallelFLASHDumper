# vim: noexpandtab

CFLAGS := -Wall -Werror

all: nordump

nordump: nordump.o
	$(CC) -o $@ $< -lwiringPi -lpthread

tags:
	ctags -R .

clean:
	rm -f *.o nordump tags
