#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h> /* pentru open(), exit() */
#include <fcntl.h> /* O_RDWR */
#include <stdlib.h>

#include "helpers.h"

void usage(char*file)
{
	fprintf(stderr,"Usage: %s ip_server port_server file\n",file);
	exit(-1);
}

/*
*	Utilizare: ./client ip_server port_server nume_fisier_trimis
*/
int main(int argc,char**argv)
{
	if (argc!=3)
		usage(argv[0]);
	
	int fd;
	struct sockaddr_in to_station;
	char buf[BUFFLEN];

	/*Deschidere socket*/
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

	if (sock < 0) {
		perror("Eroare la crearea socket-ului");
		close(sock);
		return -1;
	}
	
	/* Deschidere fisier pentru citire */
	// DIE((fd=open(argv[3],O_RDONLY))==-1, argv[3]);
	
	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	
	to_station.sin_port = htons(atoi(argv[2]));
	to_station.sin_family = AF_INET;
	inet_aton(argv[1], &to_station.sin_addr);

	/*
	*  cat_timp  mai_pot_citi
	*		citeste din fisier
	*		trimite pe socket
	*/	

	int copiat;

	// while ((copiat = read(fd, buf, sizeof(buf)))) {
    //     if (copiat < 0) {
	// 		perror("Eroare la citire");
	// 		return -1;
	// 	}

    //     int s = sendto(sock, buf, BUFFLEN, 0, (struct sockaddr*) &to_station, sizeof(to_station));

	// 	if (s < 0) {
	// 		perror("Eroare la trimiterea mesajului");
	// 		return -1;
	// 	}
    // }

	sprintf(buf, "pizza 2 -2.23");

	int s = sendto(sock, buf, BUFFLEN, 0, (struct sockaddr*) &to_station, sizeof(to_station));

	if (s < 0) {
		perror("Eroare la trimiterea mesajului");
		close(sock);
		// close(fd);
		return -1;
	}

	/*Inchidere socket*/

	close(sock);
	
	/*Inchidere fisier*/

	// close(fd);
	return 0;
}