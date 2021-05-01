#include <iostream>
#include <iomanip>
#include <math.h>
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
int fdMax, valoareRet;
char buffer[BUFFLEN], buffy[BUFFLEN];
bool runServer = true;
// pt fiecare fd, IDul clientului
unordered_map<int, string> socketIdConnection;
// 0 nu a fost deloc, 1 a fost, 2 e conectat
unordered_map<string, int> idConnected;
// pentru fiecare topic retin IDul si SF-ul
unordered_map<string, vector<pair<string, int>>> topicId;
// coada cu mesaje
unordered_map<string, vector<string>> messagesQueue;
// unde se afla fiecare clent ( pe ce fd )
unordered_map<string, int> idSocketConnection;
// valoare pt Neagle alg
const int VALUE = 1;
string message[4] = {"INT", "SHORT_REAL", "FLOAT", "STRING"};

// functie pentru nerespectarea input-ului
void portError(char *fileName) {
    fprintf(stderr, "Fisierul %s nu are un PORT specificat", fileName);
    exit(1);
}

void openSockets() {
    // construim cele 2 socket-uri : 1 pt UDP si unul pt TCP
    sockTCP = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockTCP < 0, "Socket-ul TCP nu a putut fi creat!\n");

    int binder = bind(sockTCP, (struct sockaddr *) &servAddr,
                    sizeof(servAddr));
    DIE(binder < 0, "Nu s-a putut face bind pe socket-ul TCP!");

    int listener = listen(sockTCP, SOMAXCONN / 4);
    DIE(listener < 0, "Nu s-a putut asculta pe socket-ul TCP!");

    sockUDP = socket(PF_INET, SOCK_DGRAM, 0);
    DIE(sockUDP < 0, "Socket-ul UDP nu a putut fi creat!");

    binder = bind(sockUDP, (struct sockaddr *) &servAddr, sizeof(servAddr));
    DIE(binder < 0, "Nu s-a putut face bind pe socket-ul UDP!");
}

// oresc serverul si toti clientii acestuia
void closeServer() {
    runServer = false;
    memset(buffer, 0, BUFFLEN);
    sprintf(buffer, "exit");

    for (int i = 3; i <= fdMax; ++i) {
        if (FD_ISSET(i, &readFds) && i != sockUDP && i != sockTCP) {
            int ans = send(i, buffer, sizeof(buffer), 0);
            DIE(ans < 0, "Nu se poate comunica cu clientul TCP!");
            close(i);
        }
    }

    close(sockUDP);
    close(sockTCP);
}

// verific daca deja un client este abonat la topicul respectiv
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

    // deschid socketurile
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
        valoareRet = select(fdMax + 1, &tmpFds, NULL, NULL, NULL);
        DIE(valoareRet < 0, "Eroare la selectia socket-ului de citire!");

        // parcurg toti fds pentru a afla de unde primec informatie
        for (int i = 0; i <= fdMax && runServer; ++i) {
            if (FD_ISSET(i, &tmpFds)) {
                if (i == 0) { // citesc de la STDIN comenzi
                    char reader[BUFFLEN];

                    cin.getline(reader, BUFFLEN - 1);

                    if (strcmp(reader, "exit") == 0) {
                        closeServer();
                    } else {
                        cout << "Nu exista aceasta comanda.\n";
                    }
                } else if (i == sockUDP) { // iau info de la un client UDP
                    clientLen = sizeof(clientAddr);
                    memset(&clientAddr, 0, sizeof(clientAddr));
                    char bfr[UDPBUFF];
                    int ans = -1;

                    // preiau informatia de la UDP
                    memset(bfr, 0, UDPBUFF);
                    ans = recvfrom(i, bfr, UDPBUFF, 0,
                        (struct sockaddr *) &clientAddr, &clientLen);
                    DIE(ans < 0, "Nu s-a putut prelua de la clientul UDP!");

                    udp_message msg;
                    char dataType[2];

                    strcpy(msg.topic, bfr); // topic
                    msg.tip_date = bfr[50]; // data_type

                    if (msg.tip_date == 0) { // INT cu semn
                        int sign = bfr[51];
                        long payload = ntohl(*(uint32_t*) (bfr + 52));

                        if (sign == 1) {
                            payload *= (-1);
                        }

                        strcpy(msg.payload, to_string(payload).c_str());
                    }
                    else if (msg.tip_date == 1) { // SHORT_REAL
                        double payload = 1.0 * ntohs(*(uint16_t*)(bfr + 51));
                        payload = payload / 100;
                        ostringstream strOutput;

                        if (floor(payload) == payload) {
                            strOutput << fixed << setprecision(0) << payload;
                        } else {
                            strOutput << fixed << setprecision(2) << payload;
                        }

                        strcpy(msg.payload, strOutput.str().c_str());
                    } else if (msg.tip_date == 2) { // FLOAT
                        int sign = bfr[51];
                        ostringstream strOutput;
                        double pwr = pow(10, (uint8_t) bfr[56]);
                        double output = 1.0 * (ntohl(*(uint32_t*)(bfr + 52)));
                        output = output / pwr;

                        if (sign == 1) {
                            output *= -1;
                        }

                        if (floor(output) == output) {
                            strOutput << fixed << setprecision(0) << output;
                        } else {
                            strOutput << fixed << setprecision(4) << output;
                        }

                        strcpy(msg.payload, strOutput.str().c_str());
                    } else { // STRING
                        strcpy(msg.payload, bfr + 51);
                    }

                    // construirea mesajului pentru client
                    memset(buffer, 0, BUFFLEN);
                    sprintf(buffer, "%s:%d - %s - %s - %s",
                            inet_ntoa(clientAddr.sin_addr),
                            (ntohs(clientAddr.sin_port)),
                            msg.topic, &message[msg.tip_date][0],
                            msg.payload);

                    // parcurgerea tuturor subscriberilor al acelui topic
                    for (auto it : topicId[msg.topic]) {
                        string uId = it.first;

                        // daca e onlin ii trimit mesajul
                        if (idConnected[uId] == 2) {
                            int skt = idSocketConnection[it.first];
                            int result = send(skt, buffer, BUFFLEN, 0);
                            DIE(result < 0, "Eroare la trimiterea topicului");
                        } else if (it.second == 1) {
                            // daca nu e online il pu intr-o coada
                            // pentru a-l trimite mai tarziu
                            messagesQueue[uId].push_back(string(buffer));
                        }
                    }

                } else if (i == sockTCP) { // un nou client se conecteaza
                    // il acceptam
                    acceptedSock = accept(i,
                                    (struct sockaddr *) &clientAddr,
                                    &clientLen);
                    DIE(acceptedSock < 0,
                        "Eroare la acceptarea unui nou client TCP!");

                    // dezactivam algoritum nagle
                    int nagle = setsockopt(acceptedSock, IPPROTO_TCP,
                                            TCP_NODELAY, (char *)&(VALUE),
                                            sizeof(int));
                    DIE(nagle != 0, "Nu s-a putut da disable la Nagle!");

                    // primim de la cient IDul acestuia
                    memset(buffy, 0, BUFFLEN);
                    int numberOfBytes = recv(acceptedSock, buffy, BUFFLEN, 0);
                    DIE(numberOfBytes < 0,
                        "Eroare la primirea ID-ului de la un client TCP");

                    string buff = string(buffy);

                    char answer[] = "exit";

                    // daca este deja conectat inchid coneiunea
                    // trimit mesajul de exit catre client
                    if (idConnected[buff] == 2) {
                        cout << "Client " << buff << " already connected.\n";

                        int ans = send(acceptedSock,
                                        answer,
                                        strlen(answer),
                                        0);
                        DIE(ans < 0,
                            "Nu se poate respunde clientul deja existent!");
                        close(acceptedSock);
                        continue;
                    }

                    // daca a mai fost conectat ii trimitem mesajele restante
                    if (idConnected[buff] == 1) {
                        for (auto it : messagesQueue[buff]) {
                            int result = send(acceptedSock,
                                            it.c_str(),
                                            BUFFLEN,
                                            0);
                            DIE(result < 0,
                                "Eroare trimitere topic la reconectare!");
                        }

                        messagesQueue[buff].clear();
                    }

                    FD_SET(acceptedSock, &readFds);
                    fdMax = max(fdMax, acceptedSock);

                    printf("New client %s connected from %s:%d.\n",
                                buffy, 
                                inet_ntoa(clientAddr.sin_addr),
                                ntohs(clientAddr.sin_port));

                    // retin perechea socket <-> ID client
                    socketIdConnection[acceptedSock] = buff;
                    idSocketConnection[buff] = acceptedSock;
                    idConnected[buff] = 2;
                } else { // iau info de la un client TCP
                    memset(buffer, 0, BUFFLEN);
                    // preluam informatia de la client
                    int numberOfBytes = recv(i, buffer, BUFFLEN, 0);
                    DIE(numberOfBytes < 0,
                        "Eroare primire info de la client!");

                    if (numberOfBytes == 0) { // clientul s-a deconectat
                        string id = socketIdConnection[i];

                        cout << "Client " << id << " disconnected.\n";
                        close(i);
                        FD_CLR(i, &readFds);
                        idConnected[id] = 1;
                        socketIdConnection.erase(i);
                        idSocketConnection.erase(id);
                    } else { // primit un pachet
                        string buff = string(buffer);
                        istringstream ss(buff);
                        string word;
                        string topic;
                        string SF;

                        ss >> word;

                        string backToClient = "";
                        
                        if (word == "subscribe") {// un client da subscribe
                            ss >> topic >> SF;

                            string uId = socketIdConnection[i];

                            // verificam daca clientul este deja abonat
                            if (!checkExistence(topic, uId)) {
                                topicId[topic].push_back(
                                    make_pair(uId, stoi(SF)));
                            }

                            backToClient = "Subscribed to topic.";
                        } else if (word == "unsubscribe") {// unsubscribe
                            ss >> topic;

                            // sterg utilizatorul de la topicul respectiv
                            for (int j = 0; j < topicId[topic].size(); ++j) {
                                if (socketIdConnection[i] ==
                                    topicId[topic][j].first) {
                                    topicId[topic].erase(
                                        topicId[topic].begin() + j);
                                    break;
                                }
                            }

                            backToClient = "Unsubscribed from topic.";
                        }

                        // trimit informatia inapoi
                        int ans = send(i, backToClient.c_str(), BUFFLEN, 0);
                        DIE(ans < 0, "Nu se poate trimite mesaj la TCP!");
                    }
                }
            }
        }
    }

    return 0;
}