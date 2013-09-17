#ifndef COMBAT_H
#define COMBAT_H 1
#include "globals.h"
#include <stdio.h>

struct PLAYER * sort_ships (int star);
struct PLAYER * pairing (struct PLAYER *player);
void show_combat_options (FILE * fd, struct PLAYER *player, struct PLAYER *enemy);
int find_best_range (struct PLAYER *attacker, struct PLAYER *defender);
int any_good_range (struct PLAYER *attacker, struct PLAYER *defender);
int inner_choose_target (struct ITEM *item);
int choose_target (struct ITEM *item);
int shield_rating (struct ITEM *item);
int find_longest_weapon (struct ITEM *item);
int torp_damage (struct PLAYER *player, int range);
int fire_torps (struct PLAYER *player, int range);
int damage_scored (struct PLAYER *player, int range, int phase);
void resolve_reserve_shields (FILE * fd, struct PLAYER *defender, int target);
int ship_boom (struct PLAYER *ship);
int resolve_kamikaze (FILE * fd, struct PLAYER *attacker, struct PLAYER *defender,
                  int range);
int shield_strength (struct PLAYER *player, int basic);
struct PLAYER *
resolve_strike (FILE * fd, struct PLAYER *attacker, struct PLAYER *defender,
                     int range, int phase);
int resolve_range (FILE * fd, struct PLAYER *attacker, struct PLAYER *defender,
               int range);
int resolve_retreats (FILE * fd, struct PLAYER *attacker,
                  struct PLAYER *defender, int range);
int damage_loot ();
int detection_range (struct PLAYER *attacker, struct PLAYER *defender);
int stalemate (struct PLAYER *p1, struct PLAYER *p2, int range);
void check_luck (struct PLAYER *player);
int resolve_combat (FILE * fd, struct PLAYER *attacker, struct PLAYER *defender);
void ring_clash (FILE * fd, int skill,
            struct PLAYER *attacker, struct PLAYER *defender);
int steal_skills (FILE * fd, int skill,
              struct PLAYER *attacker, struct PLAYER *defender);
int resolve_ring_interaction (FILE * fd,
                          struct PLAYER *attacker, struct PLAYER *defender);
int power (struct PLAYER *player);
int escorted (struct PLAYER *player);
struct PLAYER * link_ship (struct PLAYER *base, struct PLAYER *new);
int favour_option (struct PLAYER *player, combat_option option, int basic);


#endif
