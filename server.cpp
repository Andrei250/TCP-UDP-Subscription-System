#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_map>
#include <sstream>
#include <vector>
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
unordered_map<int, string> socketIdConnection;
unordered_map<string, bool> idConnected;
unordered_map<string, vector<int>> topicId;


void portError(char *fileName) {
    fprintf(stderr, "Fisierul %s nu are un PORT specificat\n", fileName);
    exit(1);
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
}

void closeServer() {
    runServer = false;
    memset(buffer, 0, BUFFLEN);
    sprintf(buffer, "exit");

    for (int i = 3; i <= fdMax; ++i) {
        if (FD_ISSET(i, &readFds) && i != sockUDP && i != sockTCP) {
            int ans = send(i, buffer, sizeof(buffer), 0);
            DIE(ans < 0, "Nu se poate comunica cu clientul TCP!\n");
            close(i);
        }
    }

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

                    memset(buffer, 0, BUFFLEN);
                    int numberOfBytes = recv(acceptedSock, buffer, sizeof(buffer), 0);
                    DIE(numberOfBytes < 0, "Eroare la primirea ID-ului de la un client TCP\n");

                    printf("New client %s connected from %s:%d.\n", buffer, 
                                inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

                    socketIdConnection[acceptedSock] = string(buffer);
                    idConnected[string(buffer)] = true;
                } else { // iau info de la un client TCP
                    memset(buffer, 0, BUFFLEN);
                    int numberOfBytes = recv(i, buffer, sizeof(buffer), 0);
                    DIE(numberOfBytes < 0, "Eroare la primirea informatiilor de la un client TCP!\n");

                    if (numberOfBytes == 0) {
                        string id = socketIdConnection[i];

                        cout << "Client " << id << " disconnected.\n";
                        close(i);
                        FD_CLR(i, &readFds);
                        socketIdConnection.erase(i);
                        idConnected[id] = false;
                    } else {
                        string buff = string(buffer);
                        istringstream ss(buff);
                        string word;
                        string topic;
                        string SF;

                        ss >> word;
                        
                        if (word == "subscribe") {
                            ss >> topic >> SF;

                            topicId[topic].push_back(i);
                        } else if (word == "unsubscribe") {
                            ss >> topic;
                        }

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