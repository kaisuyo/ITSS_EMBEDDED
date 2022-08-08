#ifndef __SUBHANDLE_H__
#define __SUBHANDLE_h__

typedef struct {
    int floor;
    int userID;
    SignalState dir;
} Order;

char **words(char *line, int *total, char *strCut);
void addToken(char *str, SignalState signal);
int isNumber(char *str);
int isIpV4(char *str);

#endif