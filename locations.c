#include "defs.h"
#include "globals.h"
#include "locations.h"
#include "items.h"

int
star_has_loc (int star, location_sort sort)
{
  int loc;

  for (loc = 0; loc < MAX_LOCATION; loc++)
    if (locations[loc].star == star && locations[loc].sort == sort)
      return (locations[loc].parameter);
  return (NO_LOCATION);
}

const char *
loc_string (int loc)
{
  static char buffer[256];

  sprintf (buffer, "%s", location_types[locations[loc].sort].name);
  return (buffer);
}

int
loc_type (int loc, int mask)
{
  return (location_types[locations[loc].sort].flags & mask);
}

int
loc_accessible (struct PLAYER *player, int loc)
{
  item_sort sort;

  if (!loc_type (loc, LOC_RISK))
    return (TRUE);
  switch (locations[loc].sort)
    {
    case badland:
      sort = life_support;
      break;
    case corona:
      sort = shield;
      break;
    case minefield:
      sort = cloak;
      break;
    case gas_giant:
      sort = impulse_drive;
      break;
    default:
      printf ("bad sort in loc_accessible\n");
    }
  return (factor (sort, player) > locations[loc].risk);
}

int
risk_level (struct PLAYER *player, int loc)
{
  item_sort sort;
  int result;

  if (loc <= 1)
    return (loc);               /* means not any location really */
  switch (locations[loc].sort)
    {
    case corona:
      sort = shield;
      break;
    case gas_giant:
      sort = impulse_drive;
      break;
    case minefield:
      sort = cloak;
      break;
    default:
      return (1);
    }
  result = locations[loc].risk - factor (sort, player);
  if (result < 1)
    result = 1;
  return (result);
}

int
holiday_star (int s)
{
  return (s>= MAX_STAR || s == HOLIDAY);
}
