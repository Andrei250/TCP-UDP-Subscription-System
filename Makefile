all: server subscriber

server:
	g++ server.cpp -o server

subscriber:
	g++ clientTCP.cpp -o subscriber

run_subscribers:
	./subscriber dadada 127.0.0.1 12346
	# ./subscriber nununu 127.0.0.1 12346

clean:
	rm -rf server subscriber