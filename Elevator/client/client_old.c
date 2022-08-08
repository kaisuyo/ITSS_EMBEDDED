#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#include "state.h"
#include "subHandle.h"

#define BUFF_SIZE 1000
#define MAX_FLOOR 10
char string[BUFF_SIZE];
int sockfd, currFloor = 1, lock = 1;
int user_in = 0; // 0: out, 1: in
int m; // selected option menu

int waitServer(int second) {
  // this function will loop in second s for wait server answer
  int time = 0; // time for wating
  while(1) {
    usleep(200); // 200ms for 1 frame
    if (!lock) {
      // block for wait data from server
      break;
    }
    if (time > second*1000) {
      // befor wait 2s, but lock is true then unblock
      break;
    }
    time += 200;
  }
  // if success
  if (lock == 0) {
    lock = 1;
    return 1;
  }
  // failed
  return 0;
}

void menuIn() {
  printf("0. Go out.\n");
  printf("1~%d. Go to floor.\n", MAX_FLOOR);
  printf("%d. Keep", MAX_FLOOR+1);
  printf("%d. Close", MAX_FLOOR+2);
}

void menuOut() {
  printf("0. Go in.\n");
  printf("1. Up.\n");
  printf("2. Douwn.\n");
}

pthread_t threadID;

void *socketThread() {
  pthread_detach(pthread_self());
  while(1) {
    // string: message receiving from server
    memset(string,0,strlen(string));
    if (recv(sockfd, string, BUFF_SIZE, 0) <= 0) {
      continue;
    };

    int total;
    char **data = words(string, &total, "|");
    int eleFloor = data[0][0] - '0';
    SignalState state = data[1][0] - '0';

    if (state == MSG) {
      // printf("\nFloor: %d\n", eleFloor);
    }

    if (user_in) {
      // menuIn();
    } else {
      // menuOut();
    }
  }
  return NULL;
}

int inputInt(int min, int max, char *errMessage) {
  int m;

  if (scanf("%d", &m)) {
    if (m >= min && m <= max) {
      return m;
    }
  }

  printf("\033[0;31m");
  printf("\n%s", errMessage);

  return inputInt(min, max, errMessage);
}

int main(int argc, char *argv[]) {
  // socket setup
  if(argc != 3 || !isIpV4(argv[1]) || !isNumber(argv[2])) {
    printf("INVALID ERROR.!!!");
    return 0;
  }

  int SERV_PORT = atoi(argv[2]);
  char *SERV_ADDR = argv[1];

  struct sockaddr_in servaddr;

  // create a socket for client
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Error");
    exit(2);
  }

  // creation of the remote server socket information structure
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(SERV_PORT);
  servaddr.sin_addr.s_addr = inet_addr(SERV_ADDR);
  // connect the client to the server socket
  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("Problem in connecting to the server");
    return 0;
  }
  // setup socket done

  // create thread for listen the server (current floor)
  if (pthread_create(&threadID, NULL, socketThread, NULL) != 0) {
    close(sockfd);
    printf("The elevator is having problems, please try again later.\n");
    return 0;
  };

  while(1) {
    __fpurge(stdin);
    scanf("%d", &m);

    if (user_in) {
      menuIn();
      string[0] = currFloor + '0';
      string[1] = '\0';

      if (m == 0) {
        addToken(string, USER_OUT);
      } else if (m == MAX_FLOOR+1) {
        addToken(string, KEEP);
      } else if (m == MAX_FLOOR+2) {
        addToken(string, USER_DEST);
      } else {
        addToken(string, CLOSE);
      }

    } else {
      menuOut();

      string[0] = currFloor + '0';
      string[1] = '\0';

      if (m == 0) {
        addToken(string, USER_IN);
      } else if (m == MAX_FLOOR+1) {
        // addToken(string, GOING_UP);
      } else {
        // addToken(string, GOING_DOWN);
      }
    }

    while (send(sockfd, string, strlen(string), 0) < 0);

    waitServer(5);
  }

  close(sockfd);
  return 0;
}