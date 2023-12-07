CC = gcc
CFLAGS = -Wall
EXEC = jsh

build:
	$(CC) -g $(CFLAGS) -c *.c
	$(CC) -g $(CFLAGS) -o $(EXEC) *.o -lreadline

clean:
	rm -f *.o $(EXEC)