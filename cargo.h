#ifndef CARGO_H
#define CARGO_H 1

#include "globals.h"
#include <stdio.h>

int any_cargo (struct PLAYER *player, int good);
int unload_pod (FILE * fd, struct PLAYER *player, int good);
int sell (FILE * fd, struct PLAYER *player, byte colony);
int total_cargo (struct ITEM *ship);
void shuffle_goods (struct ITEM *item, int good);
struct ITEM * any_room (struct ITEM *item);
void swap_goods (struct ITEM *ship, struct ITEM *from);
void rationalise_cargo (struct ITEM *item, int scale);
int any_available (struct ITEM *item, int good);
int inner_load_pod (struct ITEM *item, int good, int amount, int depth);
int load_pod (struct ITEM *item, int good, int amount);
void combine_goods (struct ITEM *item, int good);
void init_prices ();
struct ITEM *find_good (struct ITEM *item, int good);

#endif
