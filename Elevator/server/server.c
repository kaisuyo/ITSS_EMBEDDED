#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>

#include "state.h"
#include "subHandle.h"

#define BUFF_SIZE 100
// init
#define MAX_FLOOR 10
#define MAX_USERS 5
SignalState state = CLOSE;
int currFloor = 1;
int v = 1;
pthread_t threadID, core;

int confds[100];
int count = 0;

int orders[10];
int orderTotal = 0;

void init() {
	for (int i = 0; i < 10; i++) {
		orders[i] = 0;
	}
}

struct ThreadArgs {
	int confd;
};

int checkUp() {
	for (int i = currFloor; i < MAX_FLOOR; i++) {
		if (orders[i]) {
			return 1;
		}
	}
	return 0;
}

int checkDown() {
	for (int i = currFloor-2; i >= 0; i--) {
		if (orders[i]) {
			return 1;
		}
	}
	return 0;
}

void notifyToClients(char *message, SignalState sState) {
	addToken(message, sState);
	for (int i = 0; i < count; i++) {
		if (confds[i] == 0) continue;
		while (send(confds[i], message, strlen(message), 0) < 0);
	}
}

int hasOrders() {
	for (int i = 0; i < 10; i++) {
		if (orders[i]) return 1;
	}

	return 0;
}

void handle() {
	for (int i = 0; i < 10; i++) {
		printf("%d\t", orders[i]);
	}

	printf("\nFloor: %d\n", currFloor);

	if (!hasOrders()) {
		state = KEEP;
		v = 0;
	} else {
		if (v > 0) {
			if (!checkUp()) {
				if (!checkDown()) {
					state = KEEP;
					v = 0;
				} else {
					v = -1;
				}
			}
		} else if (v < 0) {
			if (!checkDown()) {
				if (!checkUp()) {
					state = KEEP;
					v = 0;
				} else {
					v = 1;
				}
			}
		} else {
			if (checkUp()) {
				v = 1;
			} else if (checkDown()) {
				v = -1;
			}
		}
	}

	currFloor += v;
}

void *coreThread(void *threadArgs) {
	pthread_detach(pthread_self());
	while(1) {
		// move
		handle();

		char msg[5];
		if (orders[currFloor-1]) {
			orders[currFloor-1] = 0;		
			sprintf(msg, "%d", 2);	
		} else {
			sprintf(msg, "%d", v);
		}

		strcat(msg, "|");
		char floorS[5];
		sprintf(floorS, "%d", currFloor);
		strcat(msg, floorS);
		notifyToClients(msg, MSG);

		sleep(3);
	}
	return NULL;
}

void *ThreadMain(void *threadArgs) {
	int confd;

	pthread_detach(pthread_self());

	confd = ((struct ThreadArgs *)threadArgs)->confd;
	free(threadArgs);

	char buff[BUFF_SIZE+1];
	int recvBytes;
	while (1) {
		recvBytes = recv(confd, buff, BUFF_SIZE, 0);
		if (recvBytes <= 0) {
			printf("Client is disconnect\n");
			break;
		}

		// start coding from here
		printf("%s\n", buff);
		int tokenTotal;
		char **data = words(buff, &tokenTotal, "|");
		SignalState SIGNAL = data[1][0] - '0';

		switch(SIGNAL) {
			case ORDER: {
				if (atoi(data[0]) > 0) {
					orders[atoi(data[0])-1] = 1;
				}
				break;
			}
			case USER_IN: {
				if (atoi(data[0]) == currFloor) {
					orders[atoi(data[0])-1] = 0;
					char str[5];
					sprintf(str, "%d", currFloor);
					strcat(buff, str);
					addToken(buff, USER_IN);
					send(confd, buff, strlen(buff), 0);
				}
				break;
			}
			case USER_OUT: {
				if (atoi(data[0]) == currFloor) {
					orders[atoi(data[0])-1] = 0;
					char str[5];
					sprintf(str, "%d", currFloor);
					strcat(buff, str);
					addToken(buff, USER_OUT);
					send(confd, buff, strlen(buff), 0);
				}
				break;
			}
			default: {
				// error notify
			}
		}
		memset(buff,0,strlen(buff));
	}
	close(confd);
	return NULL;
}

int main(int argc, char *argv[]) {
	if (argc != 2 || !isNumber(argv[1])) {
		printf("INVALID INPUT!!!\n");
		exit(0);
	}
	// setup argument input
	int SERV_PORT = atoi(argv[1]);
	// setup thread
	
	int listenfd;

	// setup for share memory
	socklen_t clilen;

	struct sockaddr_in cliaddr, servaddr;
	// create of server socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Error");
		return 0;
	};
	// Preparation of the socket address struct
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);
	// Bind the socket to the port in address
	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("Error");
		return 0;
	};

	// Listen for connection to the socket
	if (listen(listenfd, 5) < 0) {
		perror("Error");
		return 0;
	};

	clilen = sizeof(cliaddr);

	// thread for elevator working
	while (pthread_create(&core, NULL, coreThread, NULL) != 0);

	while (1) {
		// int sendBytes;
		// accept a connection request -> return a File Descriptor
		confds[count] = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

		// printf("Received request...\n");
		/* Create separate memory for client argument */
		struct ThreadArgs *threadArgs;
		threadArgs = (struct ThreadArgs *)malloc(sizeof(struct ThreadArgs));
		threadArgs->confd = confds[count];
		while (pthread_create(&threadID, NULL, ThreadMain, (void*)threadArgs) != 0);

		count++;
	}
	close(listenfd);
	return 0;
}