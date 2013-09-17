#ifndef RANKING_H
#define RANKING_H 1
#include <stdio.h>
#include "globals.h"

void do_rankings (FILE * fd);
void big_rank (FILE * fd, struct PLAYER *player, int index);
int big_ranking (FILE * output, int index, char *title);
void all_rankings ();

#endif
