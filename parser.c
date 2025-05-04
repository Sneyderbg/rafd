#include "afd.h"
#include "utils.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 1024

#define SIGMA_TAG "sigma"
#define Q_TAG "states"
#define Q0_TAG "init_state"
#define F_TAG "final_states"
#define DELTA_TAG "delta"

typedef enum AfdParameter {
  SIGMA = 0,
  STATES,
  INITIAL_STATE,
  FINAL_STATES,
  DELTA,
  UNKNOWN = 5
} AfdParameter;

bool parseSigma(void);
bool parseQ(void);
bool parseQ0(void);
bool parseF(void);
bool parseDelta(void);

const char *paramsNames[5] = {SIGMA_TAG, Q_TAG, Q0_TAG, F_TAG, DELTA_TAG};
bool (*parsingFuncs[5])(void) = {&parseSigma, &parseQ, &parseQ0, &parseF,
                                 &parseDelta};

typedef struct {
  AFD *parsedAFD;
  FILE *file;
  const char *filename;
  char line[MAX_LINE_LENGTH];
  size_t lineNumber;
  char *nextToken;
  char *token;
  int tokenIdx;
  char actualSep[2];
  bool parsedParams[5];
  char errorMsg[1024];
} PCtx;

PCtx parsingCtx = {0};

#define cancelParsing()                                                        \
  AFD_free(parsingCtx.parsedAFD);                                              \
  fclose(parsingCtx.file);                                                     \
  if (errorMsg != NULL) {                                                      \
    *errorMsg = parsingCtx.errorMsg;                                           \
  }                                                                            \
  return NULL

bool readLine() {
  parsingCtx.nextToken = NULL;
  parsingCtx.tokenIdx = -1;
  parsingCtx.lineNumber++;
  char *res = fgets(parsingCtx.line, MAX_LINE_LENGTH, parsingCtx.file);
  return res != NULL;
}

bool readToken() {
  if (parsingCtx.tokenIdx >= 0) {
    parsingCtx.token =
        strtok_r(NULL, parsingCtx.actualSep, &parsingCtx.nextToken);
  } else {
    parsingCtx.token =
        strtok_r(parsingCtx.line, parsingCtx.actualSep, &parsingCtx.nextToken);
  }
  parsingCtx.tokenIdx++;
  return parsingCtx.token != NULL;
}

bool parseSigma() {
  if (!readLine()) {
    sprintf(parsingCtx.errorMsg,
            "Can't parse contents for " SIGMA_TAG " at file '%s'",
            parsingCtx.filename);
    return false;
  }

  while (readToken()) {

    if (strlen(parsingCtx.token) != 1) {
      sprintf(parsingCtx.errorMsg,
              SIGMA_TAG
              " contents must have only chars of length 1: token = %s",
              parsingCtx.token);
      return false;
    }

    DA_append(parsingCtx.parsedAFD->sigma, parsingCtx.token[0]);
  }

  return true;
}

bool parseQ() {
  if (!readLine()) {
    strcpy(parsingCtx.errorMsg, "Can't parse contents for " Q_TAG);
    return false;
  }

  while (readToken()) {

    char *stateToken;
    stateToken = malloc(strlen(parsingCtx.token) * sizeof(char));
    strcpy(stateToken, parsingCtx.token);

    DA_append(parsingCtx.parsedAFD->Q, stateToken);
  }

  return true;
}

bool parseQ0() {
  if (!parsingCtx.parsedParams[STATES]) {
    strcpy(parsingCtx.errorMsg,
           Q0_TAG " requires " Q_TAG " to be first defined.");
    return false;
  }

  if (!readLine()) {
    strcpy(parsingCtx.errorMsg, "Can't parse contents for " Q0_TAG);
    return false;
  }

  while (readToken()) {
    if (parsingCtx.tokenIdx > 0) {
      // TODO: Make warning instead of error and accept only first one
      strcpy(parsingCtx.errorMsg, "There must exist only one initial state");
      return false;
    }

    int stateIdx = AFD_getStateIdx(parsingCtx.parsedAFD, parsingCtx.token);
    if (stateIdx == -1) {
      sprintf(parsingCtx.errorMsg, "%s final state not a valid state of " Q_TAG,
              parsingCtx.token);
      return false;
    }

    parsingCtx.parsedAFD->q0 = parsingCtx.parsedAFD->Q.items[stateIdx];
  }

  return true;
}

bool parseF() {
  if (!parsingCtx.parsedParams[STATES]) {
    strcpy(parsingCtx.errorMsg,
           F_TAG " requires " Q_TAG " to be first defined.");
    return false;
  }

  if (!readLine()) {
    strcpy(parsingCtx.errorMsg, "Can't parse contents for " F_TAG);
    return false;
  }

  char *stateToken;
  while (readToken()) {

    int stateIdx = AFD_getStateIdx(parsingCtx.parsedAFD, parsingCtx.token);
    if (stateIdx == -1) {
      sprintf(parsingCtx.errorMsg, "%s final state not a valid state of " Q_TAG,
              parsingCtx.token);
      return false;
    }

    stateToken = parsingCtx.parsedAFD->Q.items[stateIdx];
    DA_append(parsingCtx.parsedAFD->F, stateToken);
  }

  return true;
}

bool parseDelta() {
  if (!parsingCtx.parsedParams[SIGMA]) {
    strcpy(parsingCtx.errorMsg,
           DELTA_TAG " requires " SIGMA_TAG " to be first defined.");
    return false;
  }

  if (!parsingCtx.parsedParams[STATES]) {
    strcpy(parsingCtx.errorMsg,
           DELTA_TAG " requires " Q_TAG " to be first defined.");
    return false;
  }

  // alloc delta and connections
  size_t numStates = parsingCtx.parsedAFD->Q.len;
  size_t numInputs = parsingCtx.parsedAFD->sigma.len;

  parsingCtx.parsedAFD->delta = malloc(numStates * sizeof(char *));
  for (size_t row = 0; row < numStates; row++) {
    parsingCtx.parsedAFD->delta[row] = malloc(numInputs * sizeof(char *));
  }

  parsingCtx.parsedAFD->connections =
      malloc(numStates * numStates * sizeof(char *));
  for (size_t pos = 0; pos < numStates * numStates; pos++) {
    parsingCtx.parsedAFD->connections[pos] = calloc(
        (numInputs + 1), sizeof(char)); // +1 to allow null terminated str
  }

  // populate delta and connections
  size_t qIndex = 0;
  char *stateToken;
  while (qIndex < numStates && readLine()) {

    while (readToken() && parsingCtx.tokenIdx < (int)numInputs) {

      int nextStateIdx =
          AFD_getStateIdx(parsingCtx.parsedAFD, parsingCtx.token);
      if (nextStateIdx == -1) {
        sprintf(parsingCtx.errorMsg,
                "%s final state not a valid state of " Q_TAG, parsingCtx.token);
        return false;
      }
      stateToken = parsingCtx.parsedAFD->Q.items[nextStateIdx];

      parsingCtx.parsedAFD->delta[qIndex][parsingCtx.tokenIdx] = stateToken;
      char *inputs = parsingCtx.parsedAFD
                         ->connections[qIndex * parsingCtx.parsedAFD->Q.len +
                                       nextStateIdx];
      char *inputsEndPtr = strchr(inputs, '\0');
      *inputsEndPtr = parsingCtx.parsedAFD->sigma.items[parsingCtx.tokenIdx];
    }

    // if not enough columns for inputs
    if (parsingCtx.tokenIdx == 0) {
      sprintf(parsingCtx.errorMsg,
              "There are missing rows for " DELTA_TAG
              ", expected %zu, found %zu",
              numStates, qIndex);
      return false;
    } else if (parsingCtx.tokenIdx < (int)numInputs - 1) {
      sprintf(parsingCtx.errorMsg, DELTA_TAG "must have %zu columns.",
              numInputs);
      return false;
    } else if (parsingCtx.tokenIdx > (int)numInputs) {
      printf("WARNING: Skipping exceding columns at param " DELTA_TAG
             " in file %s\n",
             parsingCtx.filename);
    }

    qIndex++;
  }

  if (qIndex < numStates - 1) {
    sprintf(parsingCtx.errorMsg,
            "There are missing rows for " DELTA_TAG ", expected %zu, found %zu",
            numStates, qIndex);
    return false;
  }

  return true;
}

AFD *AFD_parse(const char *filename, const char sep, char **errorMsg) {
  parsingCtx = (PCtx){0}; // reset context
  parsingCtx.file = fopen(filename, "r");

  if (!parsingCtx.file) {
    sprintf(parsingCtx.errorMsg, "%s", strerror(errno));
    cancelParsing();
  }

  parsingCtx.filename = filename;
  parsingCtx.parsedAFD = calloc(1, sizeof(AFD));

  parsingCtx.actualSep[0] = sep;
  parsingCtx.actualSep[1] = '\n';

  // parse params
  bool allParsed = false;
  while (readLine()) {

    // skip other lines
    if (allParsed) {
      printf("WARNING: Skipping lines after line %zu in file %s, all params "
             "are already parsed.",
             parsingCtx.lineNumber - 1, parsingCtx.filename);
      break;
    }

    readToken();

    // skip empty lines
    if (!parsingCtx.token)
      continue;

    // skip lines starting with # -> comments
    if (parsingCtx.token[0] == '#') {
      continue;
    }

    // only compares first token and each function proceeds to the next line
    AfdParameter paramToParse = UNKNOWN;
    for (int i = 0; i < 5; i++) {
      if (strcmp(parsingCtx.token, paramsNames[i]) == 0) {
        paramToParse = i;
        break;
      }
    }

    if (paramToParse != UNKNOWN) {
      bool parsed = parsingFuncs[paramToParse]();
      if (!parsed) {
        cancelParsing();
      }
      parsingCtx.parsedParams[paramToParse] = parsed;
    }

    allParsed = parsingCtx.parsedParams[0] && parsingCtx.parsedParams[1] &&
                parsingCtx.parsedParams[2] && parsingCtx.parsedParams[3] &&
                parsingCtx.parsedParams[4];
  }

  if (!allParsed) {
    strcpy(parsingCtx.errorMsg, "There are missing parameters: ");
    for (size_t i = 0; i < 5; i++) {
      if (!parsingCtx.parsedParams[i]) {
        sprintf(parsingCtx.errorMsg, "%s,%s", parsingCtx.errorMsg,
                paramsNames[i]);
      }
    }
    cancelParsing();
  }

  fclose(parsingCtx.file);
  return parsingCtx.parsedAFD;
}
