#include "afd.h"
#include "utils.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

bool AFD_new(AFD *afd, char *sigma, int sigma_len, char **Q, int Q_len,
             char **F, int F_len, char ***delta) {
  TODO("implement");
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

bool AFD_isCurrentSuccess(AFD *afd) {
  return AFD_isStateSuccess(afd, afd->current_state);
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

// returns false if input is not in sigma, true otherwise
bool AFD_feedOne(AFD *afd, char input) {
  int inputIdx = AFD_getInputIdx(afd, input);
  if (inputIdx == -1)
    return false;

  int stateIdx = AFD_getStateIdx(afd, afd->current_state);
  afd->previous_state = afd->current_state;
  afd->current_state = afd->delta[stateIdx][inputIdx];

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
  afd->current_state = afd->q0;
  afd->previous_state = NULL;
}

void AFD_free(AFD *afd) {
  if (afd->delta != NULL) {
    for (int row = 0; row < afd->Q.len; row++) {
      for (int col = 0; col < afd->sigma.len; col++) {
        free(afd->delta[row][col]);
      }
      free(afd->delta[row]);
    }
    free(afd->delta);
  }
  if (afd->q0 != NULL) {
    free(afd->q0);
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
        if (notEmptyLineNumber > 2 && !AFD_isValidState(afd, token)) {
          AFD_free(afd);
          fclose(fileDef);
          exit(1);
        }

        char *tokenCpy = malloc(strlen(token) * sizeof(char));
        strcpy(tokenCpy, token);

        switch (notEmptyLineNumber) {
        case 2: // Q
          DA_append(afd->Q, tokenCpy);
          break;
        case 3: // q_0
          afd->q0 = tokenCpy;
          break;
        case 4: // F

          DA_append(afd->F, tokenCpy);
          break;
        default: // delta 5 -> 5+len(Q)
          if (afd->delta == NULL) {
            afd->delta = malloc(afd->Q.len * sizeof(char *));
            for (int row = 0; row < afd->Q.len; row++) {
              afd->delta[row] = malloc(afd->sigma.len * sizeof(char *));
            }
          }

          int qIndex = notEmptyLineNumber - 5;
          if (qIndex < afd->Q.len && tokenIdx < afd->sigma.len) {
            afd->delta[qIndex][tokenIdx] = tokenCpy;
          } else {
            printf("INFO: Skipping token %d at delta definition on not empty "
                   "line %d\n",
                   tokenIdx, notEmptyLineNumber);
            free(tokenCpy);
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
