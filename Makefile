IP_SERVER = 127.0.0.1

PORT = 12345

all: server subscriber udp

server:
	g++ server.cpp -o server

subscriber:
	g++ clientTCP.cpp -o subscriber

udp:
	g++ clientUDP.cpp -o udp

run_udp:
	./udp ${IP_SERVER} ${PORT}

run_subscriber:
	./subscriber dadada1 ${IP_SERVER} ${PORT}

run_subscriber2:
	./subscriber dadada2 ${IP_SERVER} ${PORT}

run_subscribers:
	./subscriber dadada ${IP_SERVER} ${PORT}
	./subscriber nununu ${IP_SERVER} ${PORT}

run_server:
	./server ${PORT}

clean:
	rm -rf server subscriber