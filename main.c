#include "afd.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARR_LEN(arr) (sizeof arr / sizeof arr[0])

int main(int argc, char *argv[]) {

  AFD *afd = AFD_parse("./def.afdd");

  char input[256];
  fgets(input, 256, stdin);

  while (strcmp(input, "exit\n") != 0) {
    size_t newLineIdx = strcspn(input, "\n");
    input[newLineIdx] = '\0';
    int res = AFD_feed(afd, input, false);
    if (res == 1) {
      printf("%s: %s\n", "Success", input);
    } else if (res == 0) {
      printf("%s %s\n", "Not success", input);
    }

    fgets(input, 256, stdin);
  }

  AFD_free(afd);
  return 0;
}
