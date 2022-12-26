CFLAGS := -Wall -Werror

all: nordump

nordump: nordump.o
	$(CC) -o $@ $< -lwiringPi -lpthread

clean:
	rm -f *.o nordump
