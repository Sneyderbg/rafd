#include "afd.h"
#include "utils.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool AFD_new(AFD *afd, char *sigma, int sigma_len, char **Q, int Q_len,
             char **F, int F_len, char ***delta) {
  TODO("implement");
  TODO("also create connections");
}

bool AFD_isValidState(AFD *afd, char *state) {
  return AFD_getStateIdx(afd, state) != -1;
}

bool AFD_isValidInput(AFD *afd, char input) {
  return AFD_getInputIdx(afd, input) != -1;
}

int AFD_getStateIdx(AFD *afd, char *state) {

  if (state == NULL)
    return -1;

  for (int i = 0; i < afd->Q.len; i++) {
    if (strcmp(afd->Q.items[i], state) == 0)
      return i;
  }
  return -1;
}

int AFD_getInputIdx(AFD *afd, char input) {

  for (int i = 0; i < afd->sigma.len; i++) {
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

bool AFD_isCurrentSuccess(AFD *afd) {
  return AFD_isStateSuccess(afd, afd->currentState);
}

bool AFD_isStateSuccess(AFD *afd, char *state) {
  if (state == NULL)
    return false;

  for (int i = 0; i < afd->F.len; i++) {
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

// returns false if input is not in sigma, true otherwise
bool AFD_feedOne(AFD *afd, char input) {
  char *next_state = AFD_getNextState(afd, input);
  if (next_state == NULL) {
    false;
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
    for (int pos = 0; pos < afd->Q.len * afd->Q.len; pos++) {
      free(afd->connections[pos]);
    }
    free(afd->connections);
  }

  if (afd->delta != NULL) {
    for (int row = 0; row < afd->Q.len; row++) {
      free(afd->delta[row]);
    }
    free(afd->delta);
  }

  DA_free(afd->F);
  DA_free(afd->Q);
  DA_free(afd->sigma);
  free(afd);
}

#define MAX_LINE_LENGTH 1024
AFD *AFD_parse(const char *filename) {
  FILE *fileDef = NULL;
  fopen_s(&fileDef, filename, "r");

  if (!fileDef) {
    printf("ERROR: %s\n", strerror(errno));
    return NULL;
  }

  AFD *afd = calloc(1, sizeof(AFD));
  int notEmptyLineNumber = 1;
  char line[MAX_LINE_LENGTH];
  while (fgets(line, MAX_LINE_LENGTH, fileDef)) {

    char *nextToken = NULL;
    char *token = strtok_s(line, " \n", &nextToken);
    char tokenIdx = 0;

    if (!token)
      continue;

    while (token) {

      if (strlen(token) <= 0)
        continue;

      // parsing sigma
      if (notEmptyLineNumber == 1) {

        if (strlen(token) != 1) {
          fprintf(stderr,
                  "ERROR: error parsing sigma in file %s, sigma must have only "
                  "chars.\n",
                  filename);
          AFD_free(afd);
          fclose(fileDef);
          exit(1);
        }

        DA_append(afd->sigma, token[0]);
      } else {

        // once Q is defined all tokens must be valid states of Q
        int stateIdx = AFD_getStateIdx(afd, token);
        if (notEmptyLineNumber > 2 && stateIdx == -1) {
          fprintf(stderr, "ERROR: %s not a valid state\n", token);
          AFD_free(afd);
          fclose(fileDef);
          exit(1);
        }

        char *stateToken;
        if (notEmptyLineNumber > 2) {
          stateToken = afd->Q.items[stateIdx];
        } else {
          stateToken = malloc(strlen(token) * sizeof(char));
          strcpy(stateToken, token);
        }

        switch (notEmptyLineNumber) {
        case 2: // Q
          DA_append(afd->Q, stateToken);
          break;
        case 3: // q_0
          afd->q0 = stateToken;
          break;
        case 4: // F

          DA_append(afd->F, stateToken);
          break;
        default: // delta 5 -> 5+len(Q)
          // initialize delta and connections
          if (afd->delta == NULL) {
            size_t numStates = afd->Q.len;
            afd->delta = malloc(numStates * sizeof(char *));
            for (int row = 0; row < numStates; row++) {
              afd->delta[row] = malloc(afd->sigma.len * sizeof(char *));
            }

            afd->connections = malloc(numStates * numStates * sizeof(char *));
            for (int pos = 0; pos < numStates * numStates; pos++) {
              afd->connections[pos] =
                  calloc((afd->sigma.len + 1), sizeof(char));
            }
          }

          int qIndex = notEmptyLineNumber - 5;
          int inputIdx = tokenIdx;
          if (qIndex < afd->Q.len && inputIdx < afd->sigma.len) {
            int nextStateIdx = AFD_getStateIdx(afd, stateToken);
            if (nextStateIdx == -1) {
              fprintf(stderr, "ERROR: this should not happen");
              AFD_free(afd);
              fclose(fileDef);
              exit(2);
            }

            afd->delta[qIndex][inputIdx] = stateToken;
            char *inputs = afd->connections[qIndex * afd->Q.len + nextStateIdx];
            char *inputsEndPtr = strchr(inputs, '\0');
            *inputsEndPtr = afd->sigma.items[inputIdx];

          } else {
            printf("INFO: Skipping token %d at delta definition on not empty "
                   "line %d\n",
                   tokenIdx, notEmptyLineNumber);
            free(stateToken);
          }
          break;
        }
      }

      token = strtok_s(NULL, " \n", &nextToken);
      tokenIdx++;
    }

    notEmptyLineNumber++;
  }

  fclose(fileDef);
  return afd;
}
