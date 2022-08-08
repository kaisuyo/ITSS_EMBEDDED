#include <curses.h>
#include <menu.h>
#include <stdio_ext.h>
#include <stdio.h>
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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define CTRLD 4

#define BUFF_SIZE 1000
char string[BUFF_SIZE];
int sockfd, lock = 1;
int currFloor = 1;
int side;

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

void floorMsg(char *floor, int clp) {
  attron(COLOR_PAIR(clp));
  move(0, 0);
  clrtoeol();
	mvprintw(0, 0, floor);
	attroff(COLOR_PAIR(clp));
	refresh();
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
    SignalState state = data[total-1][0] - '0';
    // int eleFloor = atoi(data[total-2]);
    if (state == MSG) {
      char msg[20];
      int v = atoi(data[0]);
      if (v == 1) {
        strcpy(msg, "(UP) ");
      } else if (v == -1) {
        strcpy(msg, "(DOWN) ");
      } else {
        strcpy(msg, "(WELLCOME) ");
      }

      currFloor = atoi(data[total-2]);

      strcat(msg, data[1]);
      floorMsg(msg, 3);
      lock = 0;
    } else if (state == USER_IN) {
      side = 1;
    } else if (state == USER_OUT) {
      side = 0;
    }
  }
  return NULL;
}


char *choices[] = {
    "  10  ",
    "  9  ",
    "  8  ",
    "  7  ",
    "  6  ",
    "  5  ",
    "  4  ",
    "  3  ",
    "  2  ",
    "  1  ",
    " OUT ",
    (char *)NULL,
};

char *choices_[] = {
  " UP ",
  "DOWN",
  " IN ",
  (char *)NULL,
};


int main(int argc, char *argv[]) {
  ITEM **my_items;
  int c;
  MENU *my_menu;
  WINDOW *my_menu_win;
  int n_choices, i;

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

  /* Initialize curses */
  initscr();
  start_color();
  cbreak();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  init_pair(1, COLOR_RED, COLOR_BLACK);
  init_pair(2, COLOR_CYAN, COLOR_BLACK);
  init_pair(3, COLOR_YELLOW, COLOR_BLACK);

  side = 0;
  while(1) {
    /* Create items */
    if (side == 1) {
      n_choices = ARRAY_SIZE(choices);

      my_items = (ITEM **)calloc(n_choices, sizeof(ITEM *));
      for (i = 0; i < n_choices; ++i) {
        my_items[i] = new_item(choices[i], choices[i]);
      }
    } else {
      n_choices = ARRAY_SIZE(choices_);

      my_items = (ITEM **)calloc(n_choices, sizeof(ITEM *));
      for (i = 0; i < n_choices; ++i)
        my_items[i] = new_item(choices_[i], choices_[i]);
    }

    /* Crate menu */
    my_menu = new_menu((ITEM **)my_items);

    /* Set menu option not to show the description */
    menu_opts_off(my_menu, O_SHOWDESC);

    /* Create the window to be associated with the menu */
    my_menu_win = newwin(16, 30, 2, 0);
    keypad(my_menu_win, TRUE);

    /* Set main window and sub window */
    set_menu_win(my_menu, my_menu_win);
    set_menu_sub(my_menu, derwin(my_menu_win, 12, 26, 1, 1));
    set_menu_format(my_menu, 10, 2);
    set_menu_mark(my_menu, " * ");

    /* Print a border around the main window and print a title */
    box(my_menu_win, 0, 0);
    refresh();

    /* Post the menu */
    post_menu(my_menu);
    wrefresh(my_menu_win);

    while ((c = wgetch(my_menu_win)) != 10) {
      switch (c) {
        case KEY_DOWN:
          menu_driver(my_menu, REQ_DOWN_ITEM);
          break;
        case KEY_UP:
          menu_driver(my_menu, REQ_UP_ITEM);
          break;
        case KEY_LEFT:
          menu_driver(my_menu, REQ_LEFT_ITEM);
          break;
        case KEY_RIGHT:
          menu_driver(my_menu, REQ_RIGHT_ITEM);
          break;
        wrefresh(my_menu_win);
      }
    }

  // send to server
    memset(string,0,strlen(string));
    int selectFloor = atoi(item_name(current_item(my_menu)));
    if (selectFloor > 0) {
      strcat(string, item_name(current_item(my_menu)));
      addToken(string, ORDER);
    } else {
      char str[5];
      if (side == 0) {
        sprintf(str, "%d", currFloor);
        strcat(string, str);

        if (strcmp(item_name(current_item(my_menu)), " IN ") == 0) {
          addToken(string, USER_IN);
        } else {
          addToken(string, ORDER);
        }
      } else {
        sprintf(str, "%d", currFloor);
        strcat(string, str);
        addToken(string, USER_OUT);
      }
    }
    // send username and password to server
    send(sockfd, string, strlen(string), 0);
    waitServer(5);
  }

  /* Unpost and free all the memory taken up */
  unpost_menu(my_menu);
  free_menu(my_menu);
  for (i = 0; i < n_choices; ++i)
    free_item(my_items[i]);
  endwin();

  return 0;
}