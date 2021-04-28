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

int sock, ans;
struct sockaddr_in servAddr;
socklen_t sockLen = sizeof(struct sockaddr_in);
char buffer[BUFFLEN];
fd_set readFds, tmpFds;
int fdMax;
bool runClient = true;

void closeClient() {
    runClient = false;
    close(sock);
}

int main(int argc, char ** argv) {
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
    ans = send(sock, buffer, sizeof(buffer), 0);
    DIE(ans < 0, "Eroare la transmiterea ID-ului!\n");

    while (runClient) {
        tmpFds = readFds;

        ans = select(fdMax + 1, &tmpFds, NULL, NULL, NULL);
        DIE(ans < 0, "Nu s-a putut selecta un descriptor de citire!\n");

        if (FD_ISSET(0, &tmpFds)) {
            memset(buffer, 0, BUFFLEN);
            cin.getline(buffer, BUFFLEN);
            
            if (strcmp(buffer, "exit") == 0) {
                closeClient();
            }
        } else {
            memset(buffer, 0, BUFFLEN);
			recv(sock, buffer, BUFFLEN, 0);
			
            if (strcmp(buffer, "exit") == 0) {
                closeClient();
            }
        }

    }

    close(sock);

    return 0;
}