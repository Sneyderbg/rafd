#include "afd.h"
#include "visualizer.h"
#include <stdbool.h>

int main(int argc, char *argv[]) {

  AFD *afd = AFD_parse("./def.afdd", ' ');
  run(afd);
  AFD_free(afd);
  return 0;
}
