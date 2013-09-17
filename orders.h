#ifndef ORDERS_H
#define ORDERS_H 1
#include "globals.h"

void parse_order (char *buffer, int player);
void execute_orders ();
void set_default_orders ();
void invent_order (struct PLAYER *player, skill_sort sort);
int read_orders (int player, int turn);
#endif


