#include <iostream>
#include <string>
#include <sstream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "upd.h"
#include "helpers.h"

using namespace std;

int sock, ans;
struct sockaddr_in servAddr;
socklen_t sockLen = sizeof(struct sockaddr_in);
char buffer[BUFFLEN], reader[BUFFLEN];
fd_set readFds, tmpFds;
int fdMax;
bool runClient = true;

// opresc clientul
void closeClient() {
    runClient = false;
}

int main(int argc, char ** argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    FD_ZERO(&tmpFds);
    FD_ZERO(&readFds);

    DIE(argc < 4, "Numar insuficient de argumente pt clientul TCP!");

    // pornesc socket-ul de comunicare cu serverul
    sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock < 0, "Socket-ul TCP nu s-a putut crea!");

    FD_SET(sock, &readFds);
    FD_SET(0, &readFds);
    fdMax = sock;

    // completez informatiile necesare pentru adresa
    servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(atoi(argv[3]));
	ans = inet_aton(argv[2], &servAddr.sin_addr);
	DIE(ans == 0, "Eroare inet_aton la clientul TCP");

    // ma conectez la sv
    ans = connect(sock, (struct sockaddr*) &servAddr, sockLen);
    DIE(ans < 0, "Nu s-a putut conecta clientul TCP la server!");

    // trimit idul serverului
    memset(buffer, 0, BUFFLEN);
    sprintf(buffer, "%s", argv[1]);
    ans = send(sock, buffer, strlen(buffer), 0);
    DIE(ans < 0, "Eroare la transmiterea ID-ului!");

    while (runClient) {
        tmpFds = readFds;

        // aflu de unde primesc informatie
        ans = select(fdMax + 1, &tmpFds, NULL, NULL, NULL);
        DIE(ans < 0, "Nu s-a putut selecta un descriptor de citire!");

        if (FD_ISSET(0, &tmpFds)) { // citesc de la tastatura comenzi
            memset(reader, 0, BUFFLEN);
            cin.getline(reader, BUFFLEN - 1);

            string buff = string(reader);
            istringstream ss(buff);
            string word;

            ss >> word;
            
            if (strcmp(reader, "exit") == 0) {
                closeClient();
            } else if (word == "subscribe" || word == "unsubscribe") {
                ans = send(sock, reader, BUFFLEN, 0);
                DIE(ans < 0, "Nu s-a putut trimite la server de la client!");
            } else {
                cout << "Nu exista aceasta comanda.\n";
            }
        } else { // priesc de la server informatie
            memset(buffer, 0, BUFFLEN);
            int number = -1;
            int start = 0;

            // folosesc while, deoarece din cauza unui flow mare
            // se pot trimite foarte multe informatii si trebuie luate
            // pe chunks pentru a nu le pierde
            while (start < BUFFLEN &&
                    (number = recv(sock,
                                    (&buffer) + start,
                                    BUFFLEN - start,
                                    0)) > 0) {
                start = start + number;
            }

            DIE(number < 0, "Eroare la primirea de informatie de la server!");

            if (strcmp(buffer, "exit") == 0) {
                closeClient();
            } else if (strncmp(buffer, "Subscribed", 11) == 0 || 
                        strncmp(buffer, "Unsubscribed", 13) == 0) {
                cout << buffer << "\n";
            } else if (buffer[0] != '\0' && buffer[0] != ' ') {// mesaj
                cout << buffer << "\n";
            }
        }

    }

    close(sock);
    return 0;
}