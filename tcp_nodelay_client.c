#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <errno.h>
#include <err.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <linux/input.h>

#define IGNORE_TOP 0
#define IGNORE_BOTTOM 0

#define RES_X 1200
#define RES_Y 1920

#define BITS_PER_LONG (sizeof(long)*8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)

void sendCoordinates(int server_sock, int size, struct input_event *position);

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int sock;
	int i = 0;
	int size = 0;
	int fd = -1;
	int return_val = 0;
	int x = 0, y = 0;
	int max_x = 0, max_y = 0;
	const char *event_path = "/dev/input/event1";
	char name[256] = "Unknown";
	unsigned long bits[NBITS(KEY_MAX)];

	bool ignore = false;
	struct sockaddr_in serv_addr;
	struct input_event input;
	struct input_absinfo abs;

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	fd = open(event_path, O_RDONLY);
	if (fd < 0) {
		perror("error opening event file");
		return 1;
	}

	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	fprintf(stderr, "Input device name: \"%s\"\n", name);
	ioctl(fd, EVIOCGBIT(EV_ABS, KEY_MAX), bits);

	while (max_x == 0 || max_y == 0) {
		ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &abs);
		max_x = abs.maximum;
		ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &abs);
		max_y = abs.maximum;
	}

	sock = socket(PF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		error_handling("socket() error");

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	// set tcp_nodelay option to disable naegle's algorithm
	int opt = 1;
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr))
	    == -1)
		error_handling("connect() error!");
	else
		printf("server connected\n");

	while (1) {
		// get size(number) of position coordinates
		return_val = read(fd, &input, sizeof(struct input_event));
		if (return_val < 0) {
			perror("error reading event file");
			break;
		}

		if (input.type == EV_ABS && input.code == ABS_MT_POSITION_X) {
			x = input.value * RES_X / max_x;
		}

		if (input.type == EV_ABS && input.code == ABS_MT_POSITION_Y) {
			y = input.value * RES_Y / max_y;
		}

		if (y < IGNORE_TOP || y > (RES_Y - IGNORE_BOTTOM)) {
			printf("ignore\n");
			continue;
		}

		printf("x: %d y: %d\n", x, y);

		//printf("%d %d %d\n", EV_ABS, ABS_MT_POSITION_X, ABS_MT_POSITION_Y);
		//printf("%hu, %hu, %d\n", input.type, input.code, input.value);

		sendCoordinates(sock, size, &input);
	}

	close(sock);
	printf("end client socket\n");

	return 0;

}

void sendCoordinates(int server_sock, int size, struct input_event *position)
{
	write(server_sock, position, sizeof(struct input_event));
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
