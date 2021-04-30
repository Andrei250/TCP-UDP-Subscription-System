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
char buffer[BUFFLEN];
fd_set readFds, tmpFds;
int fdMax;
bool runClient = true;

void closeClient() {
    runClient = false;
}

int main(int argc, char ** argv) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    FD_ZERO(&tmpFds);
    FD_ZERO(&readFds);

    DIE(argc < 4, "Numar insuficient de argumente pt clientul TCP!\n");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sock < 0, "Socket-ul TCP nu s-a putut crea!\n");

    FD_SET(sock, &readFds);
    FD_SET(0, &readFds);
    fdMax = sock;

    servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(atoi(argv[3]));
	ans = inet_aton(argv[2], &servAddr.sin_addr);
	DIE(ans == 0, "Eroare inet_aton la clientul TCP\n");

    ans = connect(sock, (struct sockaddr*) &servAddr, sockLen);
    DIE(ans < 0, "Nu s-a putut conecta clientul TCP la server!\n");

    memset(buffer, 0, BUFFLEN);
    sprintf(buffer, "%s", argv[1]);
    ans = send(sock, buffer, strlen(buffer), 0);
    DIE(ans < 0, "Eroare la transmiterea ID-ului!\n");

    while (runClient) {
        tmpFds = readFds;

        ans = select(fdMax + 1, &tmpFds, NULL, NULL, NULL);
        DIE(ans < 0, "Nu s-a putut selecta un descriptor de citire!\n");

        if (FD_ISSET(0, &tmpFds)) {
            memset(buffer, 0, BUFFLEN);
            cin.getline(buffer, BUFFLEN);

            string buff = string(buffer);
            istringstream ss(buff);
            string word;

            ss >> word;
            
            if (strcmp(buffer, "exit") == 0) {
                closeClient();
            } else if (word == "subscribe" || word == "unsubscribe") {
                ans = send(sock, buffer, sizeof(buffer), 0);
                DIE(ans < 0, "Nu s-a putut trimite la server de la clientul TCP!\n");

                string topic;
                ss >> topic;
            } else {
                cout << "Nu exista aceasta comanda.\n";
            }
        } else {
            memset(buffer, 0, BUFFLEN);
			recv(sock, buffer, BUFFLEN, 0);
			
            if (strcmp(buffer, "exit") == 0) {
                closeClient();
            } else if (strncmp(buffer, "Subscribed", 11) == 0 || 
                        strncmp(buffer, "Unsubscribed", 13) == 0) {
                cout << buffer << "\n";
            }
            else {
                cout << buffer << "\n";
            }
        }

    }

    close(sock);
    return 0;
}