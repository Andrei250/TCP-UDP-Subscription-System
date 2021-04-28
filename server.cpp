#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "upd.h"
#include "helpers.h"

using namespace std;

// declarare variabile necesare
int sockTCP, sockUDP, port;
struct sockaddr_in servAddr, clientAddr;
socklen_t sockLen = sizeof(struct sockaddr_in);
fd_set readFds, tmpFds;
int fdMax, valoareReturnata;
char buffer[BUFFLEN];
bool runServer = true;

void portError(char *fileName) {
    fprintf(stderr, "Fisierul %s nu are un PORT specificat", fileName);
    exit(0);
}

void openSockets() {
    // construim cele 2 socket-uri : 1 pt UDP si unul pt TCP
    sockTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockTCP < 0, "Socket-ul TCP nu a putut fi creat!");

    int binder = bind(sockTCP, (struct sockaddr *) &servAddr, sockLen);
    DIE(binder < 0, "Nu s-a putut face bind pe socket-ul TCP!");

    int listener = listen(sockTCP, SOMAXCONN);
    DIE(listener < 0, "Nu s-a putut asculta pe socket-ul TCP!");

    sockUDP = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockUDP < 0, "Socket-ul UDP nu a putut fi creat!");

    binder = bind(sockUDP, (struct sockaddr *) &servAddr, sockLen);
    DIE(binder < 0, "Nu s-a putut face bind pe socket-ul UDP!");

    // listener = listen(sockUDP, SOMAXCONN);
    // DIE(listener < 0, "Nu s-a putut asculta pe socket-ul UDP!");
}

void closeServer() {
    runServer = false;
    close(sockUDP);
    close(sockTCP);
}

int main(int argc, char **argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    // folosire gresita a serverului
    if (argc < 2) {
        portError(argv[0]);
    }

    // luam portul dat ca argument
    port = atoi(argv[1]);

    // completam informatiile serverului
    // servarul functioneaza cu IPv4, pe portul port si primeste informatii
    // de la orice IP (INADDR_ANY)
    memset((char *) &servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET; // IPv4
    servAddr.sin_port = htons(port);
    servAddr.sin_addr.s_addr = INADDR_ANY;

    openSockets();    

    // completam multimea file descriptorilor de citire cu STDIN si socketii
    FD_ZERO(&readFds);
    FD_ZERO(&tmpFds);
    FD_SET(sockTCP, &readFds);
    FD_SET(sockUDP, &readFds);
    FD_SET(0, &readFds);
    fdMax = max(sockTCP, sockUDP);



    while (runServer) {
        tmpFds = readFds;
        // selectam un file descriptor de pe care sa citim
        valoareReturnata = select(fdMax + 1, &tmpFds, NULL, NULL, NULL);
        DIE(valoareReturnata < 0, "Eroare la selectia socket-ului de citire!");

        for (int i = 0; i <= fdMax && runServer; ++i) {
            if (FD_ISSET(i, &tmpFds)) {
                if (i == 0) {
                    memset(buffer, 0, BUFFLEN);

                    cin.getline(buffer, BUFFLEN);
                    
                    if (strcmp(buffer, "exit") == 0) {
                        closeServer();
                    }
                }
            }
        }
    }

    // inchidem conexiunile
    close(sockUDP);
    close(sockTCP);

    return 0;
}