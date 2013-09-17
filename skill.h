#ifndef SKILL_H
#define SKILL_H 1
#include "globals.h"
#include <stdio.h>

int enlightened (struct PLAYER *player, skill_sort sort);
int skill_level (int skill);
int skill_bit (skill_source sort, int number);
void show_skill (FILE * fd, skill_sort skill, int level);
int effective_skill_level (struct PLAYER *player, skill_sort sort);

#endif
