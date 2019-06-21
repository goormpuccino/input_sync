#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
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
#include <linux/input.h>

#define IGNORE_TOP 0
#define IGNORE_BOTTOM 0

// Normalized input coordinates
#define NORM_X 1080
#define NORM_Y 1920

// Physical screen resolution
#define REAL_X 1080
#define REAL_Y 2340

// Custom wm size
#define WM_X 1080
#define WM_Y 1920

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)

void error_handling(char *message);

int main(int argc, char *argv[])
{
	int serv_sock;
	int clnt_sock;
	int i = 0;
	int read_len = 0;
	unsigned int x = 0, y = 0, tmp;
	char *message = (char *)malloc(sizeof(struct input_event));

	struct input_event *position =
	    (struct input_event *)malloc(sizeof(struct input_event));

	struct sockaddr_in serv_addr;
	struct sockaddr_in clnt_addr;
	socklen_t clnt_addr_size;

	char *event_path = "/dev/input/event2";
	int fd = -1;

	fd = open(event_path, O_RDWR);

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

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

	printf("client connected...\n");

	while (1) {
		// read mesage
		if ((read_len =
		     read(clnt_sock, message,
			  sizeof(struct input_event))) > 0) {
			position = (struct input_event *)message;
			if (position->type == EV_ABS
			    && position->code == ABS_MT_POSITION_X) {
				x = position->value;
				tmp = REAL_Y * WM_X / WM_Y;
				if (WM_X < REAL_X)
					x = REAL_X / 2 - tmp / 2 +
					(x * REAL_X / NORM_X * tmp / REAL_X);
				else
					x = x * REAL_X / NORM_X;
				//x = position->value * NORM_X / REAL_X;
				position->value = x;
			}

			if (position->type == EV_ABS
			    && position->code == ABS_MT_POSITION_Y) {
				y = position->value;
				tmp = REAL_X * WM_Y / WM_X;
				if (WM_Y < REAL_Y)
					y = REAL_Y / 2 - tmp / 2 +
					(y * REAL_Y / NORM_Y * tmp / REAL_Y);
				else
					y = y * REAL_Y / NORM_Y;
				//y = position->value * NORM_X / REAL_X;
				position->value = y;
			}

			printf("x: %d y: %d\n", x, y);
			write(fd, position, sizeof(struct input_event));
		} else if (read_len == 0) {
			printf("client socket closed()\n");
			break;
		} else {		// error
			printf("read error()\n");
			break;
		}
	}

	close(fd);
	close(clnt_sock);
	close(serv_sock);

	printf("end client socket\n");
	printf("end server socket\n");

	return 0;
}

void error_handling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
