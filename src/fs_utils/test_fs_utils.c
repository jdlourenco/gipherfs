#include "fs_util.h"

#define FILENAME "results/ts.output"

int main(int argc, char **argv) {

  int i = 0;
  double ts_i, ts_f;
  FILE *fp;

  char *date = get_current_time();
  printf("%s\n", date);
  free(date);
  
  // open file in append mode
  fp = fopen(FILENAME, "a");

  for(i=1; i <= 4; i++) {
    ts_i = get_current_timestamp();
    sleep(i);
    ts_f = get_current_timestamp();

    fprintf(fp, "time measured outside %lf\n", ts_f - ts_i);
    fflush(fp);
  }

  return 0;
}
