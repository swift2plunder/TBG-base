#include "defs.h"
#include "globals.h"
#include "locations.h"
#include "bytes.h"
#include "criminals.h"
#include "rand.h"
#include "crew.h"

const char *
criminal_string (int criminal)
{
  static char buffer[80];

  snprintf (buffer, 80, "%s %s", races[criminal >> 3].name,
           crim_names[criminal & 7]);
  return (buffer);
}

int
randomize_criminal (int criminal)
{
  int loc;
  do
    loc = dice (MAX_LOCATION);
  while (locations[loc].sort == none || locations[loc].criminal ||
         !(location_types[locations[loc].sort].flags & LOC_CRIMINAL));
  locations[loc].criminal = criminal;
  return loc;
}


int
relocate_criminal (int criminal)
{
  int loc, player;

  loc = randomize_criminal(criminal);
  for (player = 0; player < MAX_PLAYER; player++)
    reset_crim (players + player, criminal);
  return (loc);
}

void
interrogate (FILE * fd, struct PLAYER *player, int code)
{
  int criminal = player->prisoner;
  int level = criminal & 7;
  int new_level;
  int other_one, star, loc; 
  location_sort sort;

  if (code == 1)                /* release for bribe */
    {
      fprintf (fd,
               "<P>Released %s for bribe of $%d, becoming enemy of %s government\n",
               criminal_string (criminal),
               500 * level * level,
               races[criminal >> 3].name);
      player->energy += 500 * level * level;
      player->prisoner = 0;
      relocate_criminal (criminal);
      player->enemies |= 1 << (criminal >> 3);
      return;
    }
  fprintf (fd, "<P>Interrogated prisoner %s and learned:\n",
           criminal_string (criminal));

  if (dice(2 * level * level) < effective_skill_level (player, weaponry)
      || code == 2) 
    {
      if (dice(2 * level * level) > effective_skill_level (player, weaponry)
          || level == 7)
        {
          if (criminal & 4)         /* high level prisoner */
            new_level = (criminal & 3) + 1;
          else
            new_level = 1;
          do
            other_one = (dice (32) << 3) | new_level;
          while (other_one == criminal);
        }
      else
        {
          other_one = (criminal & ~7) | (level + 1);
        }
    }
  else
    {
      fprintf (fd, "<BR><EM>Nothing</EM>");
      return;
    }
  star = -1;
  for (loc = 0; loc < MAX_LOCATION; loc++)
    if (locations[loc].criminal == other_one)
      {
        sort = locations[loc].sort;
        star = locations[loc].star;
        loc = MAX_LOCATION;
      }
  if (star == -1)
    {
      printf ("Lost %s\n", criminal_string (other_one));
      loc = relocate_criminal (other_one);
      sort = locations[loc].sort;
      star = locations[loc].star;
    }
  fprintf (fd, "<BR>Location of %s, currently in %s at %s\n",
           criminal_string (other_one),
           location_types[sort].name, star_names[star]);
  set_crim (player, other_one);
  if (code == 2)                /* release for information */
    {
      fprintf (fd, "<BR>Released %s for information\n",
               criminal_string (criminal));
      player->prisoner = 0;
      relocate_criminal (criminal);
    }
}

void
bounty (FILE * fd, struct PLAYER *player, int loc)
{
  int criminal = locations[loc].criminal;

  if (criminal == NO_CRIMINAL)
    {
      fprintf (fd, "<P><EM>No criminals detected at %s</EM>\n",
               loc_string (loc));
      return;
    }
  if (get_crim (player, criminal))
    {
      if (ground_combat (fd, player, (criminal & 7) * 15, weaponry, FALSE))
        {
          fprintf (fd, "<P>Captured %s at %s\n",
                   criminal_string (criminal), loc_string (loc));
          if (player->prisoner)
            {
              fprintf (fd,
                       "<BR><EM>Had to release previous prisoner: %s to make room</EM>\n",
                       criminal_string (player->prisoner));
              relocate_criminal (player->prisoner);
            }
          player->prisoner = criminal;
          locations[loc].criminal = 0;
        }
    }
}

void
set_crim (struct PLAYER *player, int crim)
{
  set_bit(player->crims,crim);
}

void
reset_crim (struct PLAYER *player, int crim)
{
  reset_bit(player->crims,crim);
}

int
get_crim (struct PLAYER *player, int crim)
{
  return get_bit(player->crims, crim);
}

int
find_criminal (int crim)
{
  int loc;

  for (loc = 0; loc < MAX_LOCATION; loc++)
    {
      if (locations[loc].criminal == crim)
        return (loc);
    }
  return (-1);
}

