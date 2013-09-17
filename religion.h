#ifndef RELIGION_H
#define RELIGION_H 1


void generate_magic_options (FILE * fd, struct PLAYER *player,
                             skill_sort sort, struct PLAYER *enemy);
void generate_prophet_options (FILE * fd, struct PLAYER *player);
void add_favour (struct PLAYER *player, skill_sort skill, int amount);
void cast_spell (FILE * fd, struct PLAYER *player, int spell, int qualifier);
int give_favour (FILE * fd, skill_sort skill, struct PLAYER *player,
                 struct PLAYER *target);
void check_favour (FILE * fd, struct PLAYER *player);
void commune (FILE * fd, struct PLAYER *player, skill_sort sort);
void choose (FILE * fd, struct PLAYER *player, int target, skill_sort sort);
void write_heretic_lists ();
void write_chosen_lists ();
void show_favour ();


#endif
