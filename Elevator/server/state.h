#ifndef __STATE_H__
#define __STATE_H__

typedef enum {
  // elevator
  ORDER,
  KEEP,
  CLOSE,
  // user
  USER_IN,
  USER_OUT,
  USER_DEST,
  MSG
} SignalState;

#endif