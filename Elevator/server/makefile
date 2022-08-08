CFLAGS = -c -Wall
CC = gcc
LIBS =  -lm

server: server.o subHandle.o
	${CC} -pthread server.o subHandle.o -o server

server.o: server.c
	${CC} ${CFLAGS} server.c

subHandle.o: subHandle.c
	${CC} ${CFLAGS} subHandle.c

clean:
	rm -f *.o *~
