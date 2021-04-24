CC=gcc
CFLAGS=-std=gnu99 -Wall -Wextra -Werror -pedantic

proj2: proj2.o

%.o: %.c
	$(CC) $(CFLAGS) -c $< -lpthread -lrt

clean:
	rm -f *.o proj2 proj2.out
