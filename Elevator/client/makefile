CFLAGS = -c -Wall 
CC = gcc
LIBS =  -lm 

client: client.o subHandle.o
	${CC} -pthread client.o subHandle.o -lmenu -lncurses -o client

client.o: client.c
	${CC} ${CFLAGS} client.c

subHandle.o: subHandle.c
	${CC} ${CFLAGS} subHandle.c

clean:
	rm -f *.o *~
