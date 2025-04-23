#ifndef AFD_H
#define AFD_H

#include "utils.h"
#include <stdbool.h>

typedef struct AFD {
  DA_new(sigma, char); // only single chars
  DA_new(Q, char *);
  DA_new(F, char *);
  char ***delta; // delta[[state]][[input]] => next_state

  // conn[[state0 * len(Q) + state1]] => char[[maxlen(sigma)]] inputs
  char **connections;
  char *q0;
  char *currentState;
  char *previousState;
} AFD;

bool AFD_new(AFD *afd, char *sigma, int sigma_len, char **Q, int Q_len,
             char **F, int F_len, char ***delta);
bool AFD_isValidState(AFD *afd, char *state);
bool AFD_isValidInput(AFD *afd, char input);
int AFD_getStateIdx(AFD *afd, char *state);
int AFD_getInputIdx(AFD *afd, char input);
char *AFD_getNextState(AFD *afd, char input);
char *AFD_getConnection(AFD *afd, int state0Idx, int state1Idx);
bool AFD_isCurrentSuccess(AFD *afd);
bool AFD_isStateSuccess(AFD *afd, char *state);
bool AFD_isCurrent(AFD *afd, char *state);
bool AFD_isPrevious(AFD *afd, char *state);
bool AFD_feedOne(AFD *afd, char input);
int AFD_feed(AFD *afd, char *string, bool skipErrors);
void AFD_reset(AFD *afd);
void AFD_free(AFD *afd);

AFD *AFD_parse(const char *filename);

#endif // !AFD_H
