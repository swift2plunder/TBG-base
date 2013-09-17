#ifndef CREW_H
#define CREW_H 1
#include "globals.h"
#include <stdio.h>

int ground_combat (FILE * fd, struct PLAYER *player, int level, skill_sort sort,
                   int medical_backup);
int total_pay (struct PLAYER *player);
void check_health (FILE * fd, struct PLAYER *p);
void kill_crew (struct PLAYER *player, skill_sort sort);
void add_crew (struct PLAYER *player, skill_sort skill, int number, int level);
void show_characters (FILE * fd, struct PLAYER *player);
void recruit_rogues (FILE * fd, struct PLAYER *player, int loc);
void heal (FILE * fd, struct PLAYER *player);


#endif
