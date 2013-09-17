#ifndef DYBUK_H
#define DYBUK_H 1

#include <stdio.h>
#include "globals.h"

void logic_bomb (FILE * fd, struct PLAYER *attacker, struct PLAYER *enemy);
void bio_bomb (FILE * fd, struct PLAYER *attacker, struct PLAYER *enemy);
void bangy_bomb (FILE * fd, struct PLAYER *attacker, struct PLAYER *enemy);
int is_evil (struct PLAYER *player);
void banish_evil (FILE *fd, struct PLAYER *evil);
void resolve_evil_interaction (FILE * fd, struct PLAYER *attacker,
                               struct PLAYER *defender);
void resolve_interaction (struct PLAYER *p1, struct PLAYER *p2);
void harvest_popcorn (FILE * fd, struct PLAYER *player);
void do_hiding_damage (FILE * fd, struct PLAYER *player);
void sell_popcorn (FILE * fd, struct PLAYER *player, int amount);
void destabilize (FILE *fd, struct PLAYER *player, int loc);
void reset_popcorn ();
void do_popcorn_auction ();
void bangpedo (FILE * fd, struct PLAYER *attacker,
               struct PLAYER *enemy, int damage);
void biopedo (FILE * fd, struct PLAYER *attacker,
               struct PLAYER *enemy, int damage);
void logipedo (FILE * fd, struct PLAYER *attacker,
               struct PLAYER *enemy, int damage);
void make_evilpedos (FILE *fd, struct PLAYER *player,
                     int num, skill_sort officer);
void generate_evil_options (FILE * fd, struct PLAYER *player,
                            skill_sort officer);
void assign_dybuk ();
void check_stability ();
void check_evil (FILE * fd, struct PLAYER *player);
void update_evil (FILE * fd, struct PLAYER *player);
#endif
