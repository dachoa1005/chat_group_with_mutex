all: server client
client: client.c
	gcc -o client client.c -lpthread
server: server.c
	gcc -o server server.c -lpthread
clean:
	rm -f client server