#ifndef LOCATIONS_H
#define LOCATIONS_H 1
#include "globals.h"

int star_has_loc (int star, location_sort sort);
const char *loc_string (int loc);
int loc_type (int loc, int mask);
int loc_accessible (struct PLAYER *player, int loc);
int risk_level (struct PLAYER *player, int loc);

#endif

