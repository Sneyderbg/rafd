#include "afd.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool AFD_isValidState(AFD *afd, char *state) {
  return AFD_getStateIdx(afd, state) != -1;
}

bool AFD_isValidInput(AFD *afd, char input) {
  return AFD_getInputIdx(afd, input) != -1;
}

int AFD_getStateIdx(AFD *afd, char *state) {

  if (state == NULL)
    return -1;

  for (size_t i = 0; i < afd->Q.len; i++) {
    if (strcmp(afd->Q.items[i], state) == 0)
      return i;
  }
  return -1;
}

int AFD_getInputIdx(AFD *afd, char input) {

  if (input == '\0') {
    return -1;
  }

  for (size_t i = 0; i < afd->sigma.len; i++) {
    if (afd->sigma.items[i] == input)
      return i;
  }
  return -1;
}

// returns NULL if input is not valid
char *AFD_getNextState(AFD *afd, char input) {
  int inputIdx = AFD_getInputIdx(afd, input);
  if (inputIdx == -1)
    return NULL;

  int stateIdx = AFD_getStateIdx(afd, afd->currentState);

  return afd->delta[stateIdx][inputIdx];
}

// returns an array of inputs associated with the connection from state0 to
// state1 with length equal to sigma size
char *AFD_getConnection(AFD *afd, int state0Idx, int state1Idx) {

  // basically connections is a linear matrix
  return afd->connections[state0Idx * afd->Q.len + state1Idx];
}

bool AFD_hasConnection(AFD *afd, int state0Idx, int state1Idx) {
  char *connectionInputs = AFD_getConnection(afd, state0Idx, state1Idx);
  return strlen(connectionInputs) > 0;
}

bool AFD_isCurrentSuccess(AFD *afd) {
  return AFD_isStateSuccess(afd, afd->currentState);
}

bool AFD_isStateSuccess(AFD *afd, char *state) {
  if (state == NULL)
    return false;

  for (size_t i = 0; i < afd->F.len; i++) {
    if (strcmp(afd->F.items[i], state) == 0)
      return true;
  }
  return false;
}

bool AFD_isCurrent(AFD *afd, char *state) {
  if (afd->currentState == NULL)
    return false;

  return strcmp(afd->currentState, state) == 0;
}
bool AFD_isPrevious(AFD *afd, char *state) {

  if (afd->previousState == NULL)
    return false;

  return strcmp(afd->previousState, state) == 0;
}

bool AFD_isNext(AFD *afd, char *state, char input) {
  char *next = AFD_getNextState(afd, input);
  if (!next)
    return false;
  return strcmp(next, state) == 0;
}

// returns false if input is not in sigma, true otherwise
bool AFD_feedOne(AFD *afd, char input) {
  char *next_state = AFD_getNextState(afd, input);
  if (next_state == NULL) {
    return false;
  }

  afd->previousState = afd->currentState;
  afd->currentState = next_state;

  return true;
}

// returns -1 if error, 0 if not succ, 1 if succ
int AFD_feed(AFD *afd, char *string, bool skipErrors) {
  AFD_reset(afd);
  int i = 0;
  char input = string[i];
  while (input != '\0') {
    bool inputInSigma = AFD_feedOne(afd, input);
    if (!skipErrors && !inputInSigma) {
      fprintf(stderr, "ERROR: input %c not in sigma\n", input);
      return -1;
    }

    i++;
    input = string[i];
  }

  return AFD_isCurrentSuccess(afd);
}

void AFD_reset(AFD *afd) {
  afd->previousState = NULL;
  afd->currentState = afd->q0;
}

void AFD_free(AFD *afd) {
  if (afd->connections != NULL) {
    for (size_t pos = 0; pos < afd->Q.len * afd->Q.len; pos++) {
      if (afd->connections[pos])
        free(afd->connections[pos]);
    }
    free(afd->connections);
  }

  if (afd->delta != NULL) {
    for (size_t row = 0; row < afd->Q.len; row++) {
      if (afd->delta[row])
        free(afd->delta[row]);
    }
    free(afd->delta);
  }

  DA_free(afd->F);
  DA_free(afd->Q);
  DA_free(afd->sigma);
  free(afd);
}
