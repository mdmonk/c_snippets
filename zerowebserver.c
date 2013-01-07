/*
 * zerowebserver.c
 *
 * Pumps out zero bytes as fast as possible,
 * useful for throughput tests
 *
 * Written by Marc Liyanage (http://www.entropy.ch)
 *
 * Compile with "cc -o zerowebserver zerowebserver.c"
 * Then start with "./zerowebserver"
 *
 * It will wait for and handle a single client connection,
 * then you have to restart it.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define BLOCKSIZE 1024 * 16
#define BLOCKCOUNT 100000
#define CONTENTLENGTH BLOCKSIZE * BLOCKCOUNT
#define MAXHEADERSIZE 2000

#define PORT 		8888



int main() {

	int 	 sock_listen, sock_client, blockcount = BLOCKCOUNT;
	int 	 addrlen, one = 1;

	struct   sockaddr_in sin;
	struct   sockaddr_in pin;

	char *buffer, *header;

	buffer = malloc(BLOCKSIZE);
	bzero(buffer, BLOCKSIZE);

	header = malloc(MAXHEADERSIZE);
	bzero(header, MAXHEADERSIZE);

	snprintf(
		header,
		MAXHEADERSIZE,
		"HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: application/garbage\r\n\r\n",
		CONTENTLENGTH
	);
	
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		pexit("socket");
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(PORT);

	if (setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one))) {
		pexit("setsockopt SO_REUSEADDR");
	}

	if (bind(sock_listen, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		pexit("bind");
	}

	if (listen(sock_listen, 5) == -1) {
		pexit("listen");
	}

	if ((sock_client = accept(sock_listen, (struct sockaddr *)&pin, &addrlen)) == -1) {
		pexit("accept");
	}
	
	/* Send HTTP response header */
	send(sock_client, header, strlen(header), 0);

	/* Now send zero bytes in buffer as fast as possible */
	while (blockcount--) {
		send(sock_client, buffer, BLOCKSIZE, 0);
	}

	/* Clean up */
	close(sock_client);
	close(sock_listen);
	free(buffer);
	free(header);

}


int pexit(char *message) {
	perror(message);
	exit(1);
}