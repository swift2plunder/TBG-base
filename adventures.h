#ifndef ADVENTURES_H
#define ADVENTURES_H
#include <stdio.h>
#include "globals.h"

void set_ad (struct PLAYER *player, int ad);
void reset_ad (struct PLAYER *player, int ad);
int get_ad (struct PLAYER *player, int ad);
void show_adventure (FILE * fd, int adventure);
int try_adventure (FILE * fd, struct PLAYER *player, int adventure);
void update_adventures (struct PLAYER *player);
void show_adventures (FILE *fd, struct PLAYER *player);
void randomize_adventures ();
void reset_adventures ();
void init_adventures ();
void find_adventure (FILE * fd, struct PLAYER *player);




#endif
