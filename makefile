# makefile
# compiler

CC = gcc

CFLAGS = -Wall -g

HEADERS =  headers.h control_tower.h
OBJECTS =  main.o control_tower.o


default: main

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

main: $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ -D_REENTRANT -lm -pthread -lpthread

clean:
	-rm -f $(OBJECTS)
	-rm -f main
