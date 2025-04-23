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

const char *paramsNames[5] = {SIGMA_TAG, Q_TAG, Q0_TAG, F_TAG, DELTA_TAG};

typedef enum AfdParameter {
  SIGMA = 0,
  STATES,
  INITIAL_STATE,
  FINAL_STATES,
  DELTA
} AfdParameter;

struct {
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
} parsingCtx = {0};

void cancelParsing() {
  AFD_free(parsingCtx.parsedAFD);
  fclose(parsingCtx.file);
  exit(1);
}

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
        strtok_s(NULL, parsingCtx.actualSep, &parsingCtx.nextToken);
  } else {
    parsingCtx.token =
        strtok_s(parsingCtx.line, parsingCtx.actualSep, &parsingCtx.nextToken);
  }
  parsingCtx.tokenIdx++;
  return parsingCtx.token != NULL;
}

void parseSigma() {
  if (!readLine()) {
    printfErr("Can't parse contents for " SIGMA_TAG " at file '%s'",
              parsingCtx.filename);
    cancelParsing();
  }

  while (readToken()) {

    if (strlen(parsingCtx.token) != 1) {
      printErr(SIGMA_TAG " contents must have only chars of lenght 1.");
      printfErr("token = %s", parsingCtx.token);
      cancelParsing();
    }

    DA_append(parsingCtx.parsedAFD->sigma, parsingCtx.token[0]);
  }

  parsingCtx.parsedParams[SIGMA] = true;
}

void parseQ() {
  if (!readLine()) {
    printErr("Can't parse contents for " Q_TAG);
    cancelParsing();
  }

  while (readToken()) {

    char *stateToken;
    stateToken = malloc(strlen(parsingCtx.token) * sizeof(char));
    strcpy(stateToken, parsingCtx.token);

    DA_append(parsingCtx.parsedAFD->Q, stateToken);
  }

  parsingCtx.parsedParams[STATES] = true;
}

void parseQ0() {
  if (!parsingCtx.parsedParams[STATES]) {
    printErr(Q0_TAG " requires " Q_TAG " to be first defined.");
    cancelParsing();
  }

  if (!readLine()) {
    printErr("Can't parse contents for " Q0_TAG);
    cancelParsing();
  }

  while (readToken()) {
    if (parsingCtx.tokenIdx > 0) {
      // TODO: Make warning instead of error and accept only first one
      printErr("There must exist only one initial state");
      cancelParsing();
    }

    int stateIdx = AFD_getStateIdx(parsingCtx.parsedAFD, parsingCtx.token);
    if (stateIdx == -1) {
      printfErr("%s final state not a valid state of " Q_TAG, parsingCtx.token);
      cancelParsing();
    }

    parsingCtx.parsedAFD->q0 = parsingCtx.parsedAFD->Q.items[stateIdx];
  }

  parsingCtx.parsedParams[INITIAL_STATE] = true;
}

void parseF() {
  if (!parsingCtx.parsedParams[STATES]) {
    printErr(F_TAG " requires " Q_TAG " to be first defined.");
    cancelParsing();
  }

  if (!readLine()) {
    printErr("Can't parse contents for " F_TAG);
    cancelParsing();
  }

  char *stateToken;
  while (readToken()) {

    int stateIdx = AFD_getStateIdx(parsingCtx.parsedAFD, parsingCtx.token);
    if (stateIdx == -1) {
      printfErr("%s final state not a valid state of " Q_TAG, parsingCtx.token);
      cancelParsing();
    }

    stateToken = parsingCtx.parsedAFD->Q.items[stateIdx];
    DA_append(parsingCtx.parsedAFD->F, stateToken);
  }

  parsingCtx.parsedParams[FINAL_STATES] = true;
}

void parseDelta() {
  if (!parsingCtx.parsedParams[SIGMA]) {
    printErr(DELTA_TAG " requires " SIGMA_TAG " to be first defined.");
    cancelParsing();
  }

  if (!parsingCtx.parsedParams[STATES]) {
    printErr(DELTA_TAG " requires " Q_TAG " to be first defined.");
    cancelParsing();
  }

  // alloc delta and connections
  size_t numStates = parsingCtx.parsedAFD->Q.len;
  size_t numInputs = parsingCtx.parsedAFD->sigma.len;

  parsingCtx.parsedAFD->delta = malloc(numStates * sizeof(char *));
  for (int row = 0; row < numStates; row++) {
    parsingCtx.parsedAFD->delta[row] = malloc(numInputs * sizeof(char *));
  }

  parsingCtx.parsedAFD->connections =
      malloc(numStates * numStates * sizeof(char *));
  for (int pos = 0; pos < numStates * numStates; pos++) {
    parsingCtx.parsedAFD->connections[pos] = calloc(
        (numInputs + 1), sizeof(char)); // +1 to allow null terminated str
  }

  // populate delta and connections
  size_t qIndex = 0;
  char *stateToken;
  while (qIndex < numStates && readLine()) {

    while (readToken() && parsingCtx.tokenIdx < numInputs) {

      int nextStateIdx =
          AFD_getStateIdx(parsingCtx.parsedAFD, parsingCtx.token);
      if (nextStateIdx == -1) {
        printfErr("%s final state not a valid state of " Q_TAG,
                  parsingCtx.token);
        cancelParsing();
      }
      stateToken = parsingCtx.parsedAFD->Q.items[nextStateIdx];

      parsingCtx.parsedAFD->delta[qIndex][parsingCtx.tokenIdx] = stateToken;
      char *inputs = parsingCtx.parsedAFD
                         ->connections[qIndex * parsingCtx.parsedAFD->Q.len +
                                       nextStateIdx];
      char *inputsEndPtr = strchr(inputs, '\0');
      *inputsEndPtr = parsingCtx.parsedAFD->sigma.items[parsingCtx.tokenIdx];
    }

    // if not enough inputs
    if (parsingCtx.tokenIdx < numInputs - 1) {
      printfErr("Delta must be have %zu columns.", numInputs + 1);
      cancelParsing();
    } else if (parsingCtx.tokenIdx > numInputs) {
      printf("WARNING: Skipping exceding columns at param " DELTA_TAG
             " in file %s\n",
             parsingCtx.filename);
    }

    qIndex++;
  }

  parsingCtx.parsedParams[DELTA] = true;
}

AFD *AFD_parse(const char *filename, const char sep) {
  fopen_s(&parsingCtx.file, filename, "r");

  if (!parsingCtx.file) {
    printfErr("%s", strerror(errno));
    return NULL;
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
    if (strcmp(parsingCtx.token, SIGMA_TAG) == 0) {
      parseSigma();
    } else if (strcmp(parsingCtx.token, Q_TAG) == 0) {
      parseQ();
    } else if (strcmp(parsingCtx.token, F_TAG) == 0) {
      parseF();
    } else if (strcmp(parsingCtx.token, Q0_TAG) == 0) {
      parseQ0();
    } else if (strcmp(parsingCtx.token, DELTA_TAG) == 0) {
      parseDelta();
    }

    allParsed = parsingCtx.parsedParams[0] && parsingCtx.parsedParams[1] &&
                parsingCtx.parsedParams[2] && parsingCtx.parsedParams[3] &&
                parsingCtx.parsedParams[4];
  }

  if (!allParsed) {
    printErr("There are missing parameters: ");
    for (size_t i = 0; i < 5; i++) {
      if (!parsingCtx.parsedParams[i]) {
        printfErr("  - %s", paramsNames[i]);
      }
    }
    printErr("Canceling parsing");
    cancelParsing();
  }

  fclose(parsingCtx.file);
  return parsingCtx.parsedAFD;
}
