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
int sockTCP, sockUDP, port, acceptedSock;
struct sockaddr_in servAddr, clientAddr;
socklen_t sockLen = sizeof(struct sockaddr_in);
socklen_t clientLen = sizeof(struct sockaddr_in);
fd_set readFds, tmpFds;
int fdMax, valoareReturnata;
char buffer[BUFFLEN];
bool runServer = true;

void portError(char *fileName) {
    fprintf(stderr, "Fisierul %s nu are un PORT specificat\n", fileName);
    exit(0);
}

void openSockets() {
    // construim cele 2 socket-uri : 1 pt UDP si unul pt TCP
    sockTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockTCP < 0, "Socket-ul TCP nu a putut fi creat!\n");

    int binder = bind(sockTCP, (struct sockaddr *) &servAddr, sockLen);
    DIE(binder < 0, "Nu s-a putut face bind pe socket-ul TCP!\n");

    int listener = listen(sockTCP, SOMAXCONN);
    DIE(listener < 0, "Nu s-a putut asculta pe socket-ul TCP!\n");

    sockUDP = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sockUDP < 0, "Socket-ul UDP nu a putut fi creat!\n");

    binder = bind(sockUDP, (struct sockaddr *) &servAddr, sockLen);
    DIE(binder < 0, "Nu s-a putut face bind pe socket-ul UDP!\n");

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
        DIE(valoareReturnata < 0, "Eroare la selectia socket-ului de citire!\n");

        for (int i = 0; i <= fdMax && runServer; ++i) {
            if (FD_ISSET(i, &tmpFds)) {
                if (i == 0) { // citesc de la STDIN comenzi
                    memset(buffer, 0, BUFFLEN);

                    cin.getline(buffer, BUFFLEN);
                    
                    if (strcmp(buffer, "exit") == 0) {
                        closeServer();
                    }
                } else if (i == sockUDP) { // iau info de la un client UDP
                    continue;
                } else if (i == sockTCP) { // un nou client se conecteaza
                    acceptedSock = accept(i, (struct sockaddr *) &clientAddr, &clientLen);
                    DIE(acceptedSock < 0, "Eroare la acceptarea unui nou client TCP!\n");

                    FD_SET(acceptedSock, &readFds);
                    fdMax = max(fdMax, acceptedSock);

                    printf("New client %d connected from %s:%d.\n", acceptedSock, 
                                inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                } else { // iau info de la un client TCP
                    memset(buffer, 0, BUFFLEN);
                    int numberOfBytes = recv(i, buffer, sizeof(buffer), 0);
                    DIE(numberOfBytes < 0, "Eroare la primirea informatiilor de la un client TCP!\n");

                    if (numberOfBytes == 0) {
                        printf("CLient %d disconnected.\n", i);
                        close(i);
                        FD_CLR(i, &readFds);
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