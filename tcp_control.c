#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void error_handling(char *message);

int main(int argc, char **argv)
{
	int serv_sock;
	int clnt_sock;
	size_t s_read;
	char readbuf[64];
	int x, y, density;
	char wmarg[1024];

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	// Disable buffering on stdout/stderr
	setbuf(stdout, NULL);
	setbuf(stderr, NULL);

	while (true) {
		serv_sock = socket(PF_INET, SOCK_STREAM, 0);
		if (serv_sock == -1)
			error_handling("socket() error");

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		serv_addr.sin_port = htons(atoi(argv[1]));

		if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))
		    == -1)
			error_handling("bind() error");

		if (listen(serv_sock, 5) == -1)
			error_handling("listen() error");

		clnt_addr_size = sizeof(clnt_addr);
		clnt_sock =
		    accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);

		if (clnt_sock == -1)
			error_handling("accept() error");

		memset(&readbuf, 0, sizeof(readbuf));
		read(clnt_sock, readbuf, 14);

		printf("input: read: %s\n", readbuf);

		// Parse X and Y from readbuf
		readbuf[4] = '\0';
		readbuf[9] = '\0';
		readbuf[13] = '\0';
		x = atoi(readbuf);
		y = atoi(readbuf + 5);
		density = atoi(readbuf + 10);

		memset(wmarg, 0, sizeof(wmarg));
		sprintf(wmarg, "wm size %dx%d", x, y);
		printf("WMARG: %s\n", wmarg);
		system(wmarg);

		memset(wmarg, 0, sizeof(wmarg));
		sprintf(wmarg, "wm density %d", density);
		printf("WMARG: %s\n", wmarg);
		system(wmarg);

		close(clnt_sock);
		close(serv_sock);

		printf("end client socket\n");
		printf("end server socket\n");

		sleep(1);
	}

	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
