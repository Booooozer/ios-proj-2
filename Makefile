CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic

proj2: proj2.o
	$(CC) $(CFLAGS) $^ -o $@ -pthread -g

%.o: %.c
	$(CC) $(CFLAGS) -c $<

zip:
	zip proj2.zip Makefile *.c

clean:
	rm -f *.o proj2 proj2.out vgcore*
