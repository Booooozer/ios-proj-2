CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic

proj2: proj2.o
	$(CC) $(CFLAGS) $^ -o $@ -lpthread -lrt -pthread -g

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o proj2 proj2.out vgcore*
