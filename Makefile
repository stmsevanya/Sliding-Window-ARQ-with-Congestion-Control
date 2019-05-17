all : server client

server : server.c
	gcc server.c -lssl -lcrypto -o server

client : client.c
	gcc client.c -lssl -lcrypto -o client

clean :
	rm server client
