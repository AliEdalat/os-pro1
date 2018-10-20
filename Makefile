server: server
	gcc server.c -o server
client: client
	gcc selclient.c -o client
.: server client

clean:
	rm client server 