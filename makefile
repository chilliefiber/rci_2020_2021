#  Compiler
CC = gcc

#  Compiler Flags
CFLAGS= -Wall -g

#  Sources
SOURCES = errcheck.h udp_parser.c input.c main.c

#  Objects
OBJECTS = errcheck.o udp_parser.o input.o main.o

ndn: main.o input.o udp_parser.o errcheck.o
	gcc $(CFLAGS) -o ndn main.o input.o udp_parser.o errcheck.o

main.o: main.c input.h udp_parser.h errcheck.h
	gcc $(CFLAGS) -c main.c

input.o: input.c input.h errcheck.h
	gcc $(CFLAGS) -c input.c

udp_parser.o: udp_parser.c udp_parser.h errcheck.h
	gcc $(CFLAGS) -c udp_parser.c

errcheck.o: errcheck.c errcheck.h
	gcc $(CFLAGS) -c errcheck.c


clean:
	rm -f *.o *.~ ndn *.gch
