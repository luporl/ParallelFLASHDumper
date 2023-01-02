# vim: noexpandtab

TEST := 1
CFLAGS := -Wall -Werror -DTEST=$(TEST)

ifeq ($(TEST),0)
TEST_O :=
else
TEST_O := test.o
endif

all: nordump

nordump: nordump.o $(TEST_O)
	$(CC) -o $@ $< $(TEST_O) -lwiringPi -lpthread

tags:
	ctags -R .

clean:
	rm -f *.o nordump tags
