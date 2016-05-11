CC=gcc
CFLAG=-std=gnu99 -lpthread

server: server.o
	${CC} $^ -o $@ ${CFLAG}
server.o:server.c
	${CC} -c $^ -o $@ ${CFLAG}

client: client.o
	${CC} $^ -o $@ ${CFLAG}
client.o: client.c
	${CC} -c $^ -o $@ ${CFLAG}

clean:
	rm *.o server client
