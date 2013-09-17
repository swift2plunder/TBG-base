#ifndef POLITICS_H
#define POLITICS_H 1
#include <stdio.h>

int minister_mod (struct PLAYER *player, int value, item_sort sort);
int aliens_report (int star, int minister, int turn);
void alien_reports ();
void generate_presidential_options (FILE * fd, struct PLAYER *player);
void generate_voting_options (FILE * fd, struct PLAYER *player);
void political_command (struct PLAYER *player, char command, int param);
void make_president_link (FILE * fd, int minister);
void check_votes (FILE * fd, struct PLAYER *player);
void consolidate_votes ();
void do_election ();
void do_tribunal_election ();
void resolve_judgement ();
void influence_decay ();

#endif










