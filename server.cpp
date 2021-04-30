#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/tcp.h>
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
char buffer[BUFFLEN], buffy[BUFFLEN];
bool runServer = true;
unordered_map<int, string> socketIdConnection;
unordered_map<string, int> idConnected;// 0 nu a fost deloc, 1 a fost, 2 e conectat
unordered_map<string, vector<pair<string, int>>> topicId;
unordered_map<string, vector<string>> messagesQueue;
unordered_map<string, int> idSocketConnection;
const int VALUE = 1;
string message[4] = {"INT", "SHORT REAL", "FLOAT", "STRING"};

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

bool checkExistence(string topic, string client) {
    for (auto it : topicId[topic]) {
        if (it.first == client) {
            return true;
        }
    }

    return false;
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
                    } else {
                        cout << "Nu exista aceasta comanda.\n";
                    }
                } else if (i == sockUDP) { // iau info de la un client UDP
                    continue;
                    clientLen = sizeof(clientAddr);
                    memset(&clientAddr, 0, sizeof(clientAddr));
                    memset(buffer, 0, BUFFLEN);      
                    int ans = recvfrom(i, (char *)buffer, BUFFLEN, 0, (struct sockaddr *) &clientAddr, &clientLen);
                    DIE(ans < 0, "Nu s-a putut prelua mesajul de la clientul UDP!\n");

                    udp_message msg;
                    char dataType[2];

                    // sscanf(buffer, "%s %s %s", msg.topic, dataType, msg.payload);
                    // msg.tip_date = dataType[0];
                    udp_message * msg2 = (udp_message*) buffer;

                    strcpy(msg.topic, buffer); // topic
                    msg.tip_date = buffer[50]; // data_type

                    char mess[50];
                    strcpy(mess, "DADADA");

                    for (int j = 3; j <= fdMax; ++j) {
                        if (FD_ISSET(j, &readFds)) {
                            if (msg2->tip_date == 0 && j != sockTCP && j != sockUDP) {
                                send(j, mess, strlen(mess), 0);
                            }
                        }
                    }
                    

                    // if (msg.tip_date == 0) { // INT cu semn
                    //     int sign = buffer[51];
                    //     long payload = ntohl(*(uint32_t*) (buffer + 52));

                    //     if (sign == 1) {
                    //         payload *= (-1);
                    //     }

                    //     strcpy(msg.payload, to_string(payload).c_str());
                    // }
                    // else if (msg.tip_date == 1) { // SHORT_REAL
                    //     float payload = ntohs(*(uint16_t*)(buffer + 51));
                    //     payload = ((float) payload) / 100;

                    //     strcpy(msg.payload, to_string(payload).c_str());
                    // } else if (msg.tip_date == 2) {
                    //     int sign = buffer[51];
                    //     int power;
                    //     double payload = ntohl(*(uint32_t*)(buffer + 52));
                    //     char powerChar = buffer[56];
                    //     power = (int)powerChar;
                    //     double divider = 1;
                    //     for (int i = 0; i < power; i++) {
                    //         divider *= 10;
                    //     }
                    //     float floatNumber = ((float)payload) / divider;
                    //     if (sign == 1) {
                    //         floatNumber *= -1;
                    //     }

                    //     strcpy(msg.payload, to_string(payload).c_str());
                    // } else {
                    //     strcpy(msg.payload, buffer + 51);
                    // }

                    // memset(buffer, 0, BUFFLEN);
                    // sprintf(buffer, "%s:%d - %s - %s - %s\n", inet_ntoa(clientAddr.sin_addr),
                    //                         (ntohs(clientAddr.sin_port)),
                    //                         msg.topic, &message[msg.tip_date][0], msg.payload);


                    // for (auto it : topicId[msg.topic]) {
                    //     string uId = it.first;

                    //     if (idConnected[uId] == 2) {
                    //         int skt = idSocketConnection[it.first];
                    //         int result = send(skt, buffer, sizeof(buffer), 0);
                    //         DIE(result < 0, "Eroare la trimiterea topicului!");
                    //     } else if (it.second == 1) {
                    //         messagesQueue[uId].push_back(string(buffer));
                    //     }
                    // }

                } else if (i == sockTCP) { // un nou client se conecteaza
                    acceptedSock = accept(i, (struct sockaddr *) &clientAddr, &clientLen);
                    DIE(acceptedSock < 0, "Eroare la acceptarea unui nou client TCP!\n");

                    int nagle = setsockopt(acceptedSock, IPPROTO_TCP,
                                            TCP_NODELAY, (char *)&(VALUE),
                                            sizeof(int));
                    DIE(nagle != 0, "Nu s-a putut da disable la Nagle!\n");

                    memset(buffer, 0, BUFFLEN);
                    int numberOfBytes = recv(acceptedSock, buffer, sizeof(buffer), 0);
                    DIE(numberOfBytes < 0, "Eroare la primirea ID-ului de la un client TCP\n");

                    string buff = string(buffer);

                    char answer[] = "exit";

                    if (idConnected[buff] == 2) {
                        cout << "Client " << buff << " already connected.\n";

                        int ans = send(acceptedSock, answer, sizeof(answer), 0);
                        DIE(ans < 0, "Nu se poate respunde clientul TCP deja existent!\n");
                        close(acceptedSock);
                        continue;
                    }

                    if (idConnected[buff] == 1) {
                        for (auto it : messagesQueue[buff]) {
                            int result = send(acceptedSock, it.c_str(), sizeof(it), 0);
                            DIE(result < 0, "Eroare la trimiterea topicului la reconectarea clientului TCP!");
                        }

                        messagesQueue[buff].clear();
                    }

                    FD_SET(acceptedSock, &readFds);
                    fdMax = max(fdMax, acceptedSock);

                    printf("New client %s connected from %s:%d.\n", buffer, 
                                inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

                    socketIdConnection[acceptedSock] = buff;
                    idSocketConnection[buff] = acceptedSock;
                    idConnected[buff] = 2;
                } else { // iau info de la un client TCP
                    memset(buffer, 0, BUFFLEN);
                    int numberOfBytes = recv(i, buffer, sizeof(buffer), 0);
                    DIE(numberOfBytes < 0, "Eroare la primirea informatiilor de la un client TCP!\n");

                    if (numberOfBytes == 0) {
                        string id = socketIdConnection[i];

                        for (auto it : messagesQueue[id]) {
                            cout << it << "\n";
                        }

                        cout << "Client " << id << " disconnected.\n";
                        close(i);
                        FD_CLR(i, &readFds);
                        idConnected[id] = 1;
                        socketIdConnection.erase(i);
                        idSocketConnection.erase(id);
                    } else {
                        string buff = string(buffer);
                        istringstream ss(buff);
                        string word;
                        string topic;
                        string SF;

                        ss >> word;

                        string backToClient = "";
                        
                        if (word == "subscribe") {
                            ss >> topic >> SF;

                            if (!checkExistence(topic, socketIdConnection[i])) {
                                topicId[topic].push_back(make_pair(socketIdConnection[i], stoi(SF)));
                            }

                            backToClient = "Subscribed to topic.";
                        } else if (word == "unsubscribe") {
                            ss >> topic;

                            for (int j = 0; j < topicId[topic].size(); ++j) {
                                if (socketIdConnection[i] == topicId[topic][j].first) {
                                    topicId[topic].erase(topicId[topic].begin() + j);
                                    break;
                                }
                            }

                            backToClient = "Unsubscribed from topic.";
                        }

                        int ans = send(i, backToClient.c_str(), strlen(backToClient.c_str()), 0);
                        DIE(ans < 0, "Nu se poate trimite mesaj la TCP!\n");
                    }
                }
            }
        }
    }

    return 0;
}