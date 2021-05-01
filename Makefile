IP_SERVER = 127.0.0.1

PORT = 23456

all: server subscriber

server:
	g++ server.cpp -o server

subscriber:
	g++ clientTCP.cpp -o subscriber

clean:
	rm -rf server subscriber