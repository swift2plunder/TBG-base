#ifndef ITEMS_H
#define ITEMS_H 1
#include "globals.h"

int find_free_item ();
short new_item (short sort, short efficiency, short reliability, short collection,
                short demo);
short add_item (struct PLAYER *ship, short new);
short generate_item (int tech, int demo);
short find_owner (short item);
void remove_item (int item);
void destroy_item (int item);
struct ITEM *lucky_item (struct PLAYER *player, item_sort sort);
void check_reliability (FILE * fd, struct PLAYER *player);
void repair (FILE * fd, struct PLAYER *player, struct ITEM *item);
void maintain (FILE * fd, struct PLAYER *player, struct ITEM *item);
void priority (FILE * fd, struct PLAYER *player, skill_sort officer);
const char * item_string (struct ITEM *item);
int transfer_item (int item, struct PLAYER *new_owner);
void transmute_items (struct PLAYER *ship, item_sort from, item_sort to);
void generate_ship (struct PLAYER *ship, int tech, int extras, int demo);
int is_weapon (item_sort sort);
int mass (struct PLAYER *player);
int owner (struct PLAYER *player, int item);
int factor (item_sort, struct PLAYER *);
void destroy_ship (struct PLAYER *ship);
struct ITEM *lucky_item (struct PLAYER *player, item_sort sort);

#endif
