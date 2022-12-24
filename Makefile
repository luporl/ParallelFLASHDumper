CFLAGS := -Wall -Werror

all: nordump

nordump: ParallelFLASHDumper_rpi.o
	$(CC) -o $@ $< -lwiringPi -lpthread

clean:
	rm -f *.o nordump
