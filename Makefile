EXEC = jsh

build:
		gcc -Wall -g -c *.c
		gcc -o $(EXEC) *.o -lreadline

clean:
		rm -f *.o
		rm -f $(EXEC)