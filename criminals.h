#ifndef CRIMINALS_H
#define CRIMINALS_H 1
#include <stdio.h>
#include "globals.h"

int randomize_criminal (int criminal);
int relocate_criminal (int criminal);
const char * criminal_string (int criminal);
void bounty (FILE * fd, struct PLAYER *player, int loc);
void interrogate (FILE * fd, struct PLAYER *player, int code);
void set_crim (struct PLAYER *player, int crim);
void reset_crim (struct PLAYER *player, int crim);
int get_crim (struct PLAYER *player, int crim);
int find_criminal (int crim);

#endif


