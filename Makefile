CC= /usr/bin/gcc

all:	group04-client group04-server

group04-client: group04-client.c;
	${CC} group04-client.c -g -o group04-client

group04-server: group04-server.c;
	${CC} group04-server.c -g -o group04-server

clean:
	rm group04-client group04-server
