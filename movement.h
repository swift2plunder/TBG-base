#ifndef MOVEMENT_H
#define MOVEMENT_H 1

#include "globals.h"

int jump_cost (int ship, int s1, int s2);
double inverse_jump_cost (int ship, int c);
void jump (FILE * fd, struct PLAYER *player, int from, int to);
int star_seen (struct PLAYER *p, int star);
void show_destinations (FILE * fd, int self, int current,
                        double min, double max, int showcost);
int square_distance (int s1, int s2);
int distance (int s1, int s2);
int get_random_star (struct PLAYER *player);


#endif
