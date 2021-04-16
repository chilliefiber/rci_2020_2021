TARGET = ndn
#  Compiler
CC = gcc

#  Compiler Flags
CFLAGS= -Wall -g

HEADERS = $(wildcard *.h)
OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

.DEFAULT_GOAL := $(TARGET)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^

clean:
	-rm -f $(TARGET) *.o core a.out *-
