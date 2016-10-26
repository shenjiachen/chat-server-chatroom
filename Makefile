client.o: server.o
	gcc  -o  client  client.c
server.o: 
	gcc  -o  server  server.c
clean:
	rm -rf server
	rm -rf client