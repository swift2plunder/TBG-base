#include <stdio.h>
#include "globals.h"
#include "adventures.h"
#include "items.h"
#include "skill.h"
#include "crew.h"
#include "rand.h"
#include "religion.h"
#include "locations.h"
#include "tbg-big.h"

void
generate_rumour (FILE * fd, struct PLAYER *player, int sort)
{
  int ad, star = dice (MAX_STAR);

  do
    ad = dice (MAX_ADVENTURE);
  while (adventures[ad].star < 0 || !star_seen(player, star));
  /* don't reveal unknown stars */

  switch (sort)
    {
    case 0:
      fprintf (fd,
               "<li>You learn of an adventure in %s at %s. The treasure is: %s %s</li>\n",
               loc_string (adventures[ad].loc),
               star_names[adventures[ad].star],
               tech_level_names[(items + adventures[ad].treasure)->efficiency],
               item_string (items + adventures[ad].treasure));
      set_ad (player, ad);
      break;
    case 1:
      fprintf (fd, "<li>You learn of the situation at %s</li>\n",
               star_names[star]);
      fprintf (fd, "<hr>\n");
      show_starsystem (fd, player, star);
      fprintf (fd, "<hr>\n");
      break;
    }
}

void
show_adventure (FILE * fd, int adventure)
{
  int parameter = adventures[adventure].parameter;

  fprintf (fd, "%s in %s, needs skill %d (%d%%)\n",
           ad_types[ADVENTURE_TYPE (parameter)].ad_name,
           skill_names[ADVENTURE_SKILL (parameter)],
           ADVENTURE_LEVEL (parameter), adventures[adventure].obscurity);
}

void
show_adventures (FILE *fd, struct PLAYER *player)
{
  int i;
  int star = player->star;
  
  for (i = 0; i < MAX_ADVENTURE; i++)
    if (adventures[i].star == star && get_ad (player, i))
      {
        fprintf (fd, "<tr><td>%d</td><td>", i);
        show_adventure (fd, i);
      }
}

void
randomize_adventure (int i)
{
  adventures[i].parameter = i;
  do
    adventures[i].loc = dice (MAX_LOCATION);
  while (locations[adventures[i].loc].sort !=
         ad_types[ADVENTURE_TYPE (i)].loc);
  adventures[i].star = locations[adventures[i].loc].star;
  adventures[i].treasure =
    generate_item (ADVENTURE_LEVEL (i) / 7 + dice (2) + 1, FALSE);
  adventures[i].obscurity = rand_exp(ADVENTURE_LEVEL(i)*ADVENTURE_LEVEL(i)/3);
  //adventures[i].obscurity = dice (100);
  adventures[i].bonus_flags = dice (31);
}


void
randomize_adventures ()
{
  int i;

  for (i = 0; i < MAX_ADVENTURE; i++)
    randomize_adventure(i);
}

void
reset_adventure (int i)
{
  int player;
  randomize_adventure(i);
  for (player = 0; player < MAX_PLAYER; player++)
    reset_ad (players + player, i);
}


void
reset_adventures ()
{
  int i;

  for (i = 0; i < MAX_ADVENTURE; i++)
    if (adventures[i].treasure == 0)
      {
        reset_adventure(i);
      }
}

int
try_adventure (FILE * fd, struct PLAYER *player, int adventure)
{
  int parameter = adventures[adventure].parameter;
  int treasure = adventures[adventure].treasure;
  skill_sort skill = ADVENTURE_SKILL (parameter);
  int flags = adventures[adventure].bonus_flags;
  int level = ADVENTURE_LEVEL (parameter);
  int i, risk;

  fprintf (fd, "<aside><h2>%s officer attempts %s at %s</h2>\n",
           skill_names[skill], ad_types[ADVENTURE_TYPE (parameter)].ad_name,
           star_names[player->star]);
  fprintf (fd, "%s", ad_types[ADVENTURE_TYPE (parameter)].ad_desc);
  if (treasure == 0 || adventures[adventure].star != player->star)
    {
      fprintf (fd, "<p>Adventure already done this turn</p></aside>\n");
      return (FALSE);
    }
  if (effective_skill_level (player, skill) < level)
    {
      fprintf (fd,
               "<p>Adventure too difficult, %d needs to be %d</p></aside>\n",
               effective_skill_level (player, skill), level);
      return (FALSE);
    }
  if (flags & BONUS_MEDICAL)
    {
      if (player->crew[skill] < level / 2)
        {
          fprintf (fd, "<p>Adventure needs at least %d crew</p></aside>\n",
                   level / 2);
          return (FALSE);
        }
      risk = level * 3 + 1;
      fprintf (fd, "<p>Risk of fatal accident is %d%% for each of %d crew</p>\n",
               risk, level / 2);
      if (player->away_team[medical] == parameter)
        {
          if (player->magic_flags & FLAG_BLESS_MEDICAL)
            risk -= (3 * effective_skill_level (player, medical)) / 2;
          else
            risk -= effective_skill_level (player, medical);
          if (risk < 0)
            risk = 0;
          fprintf (fd, "<p>Risk reduced to %d%% by medical crew</p>\n", risk);
        }
      for (i = 0; i < level / 2; i++)
        if (dice (100) < risk)
          {
            fprintf (fd, "<p>Oops, lost one %s crew!</p>\n",
                     skill_names[skill]);
            kill_crew (player, skill);
          }
    }
  if (flags & BONUS_WEAPONARY)
    {
      fprintf (fd, "<p>Combat needed to resolve adventure</p>\n");
      if (!ground_combat
          (fd, player, 4 * level,
           player->away_team[weaponry] == parameter ? weaponry : skill,
           player->away_team[medical] == parameter))
        {
          fprintf (fd,
                   "<p>Retreat from combat, adventure failed</p></aside>\n");
          return (FALSE);
        }
    }
  fprintf (fd, "<h3>Succeeded at adventure!</h3>\n");
  if ((flags & BONUS_ENGINEERING)
      && (player->away_team[engineering] == parameter)
      && items[treasure].sort != pod)
    {
      fprintf (fd, "<p>Engineering crew help salvage treasure module</p>\n");
      items[treasure].reliability +=
        effective_skill_level (player, engineering);
      if (items[treasure].reliability > 99)
        items[treasure].reliability = 99;
    }
  fprintf (fd, "<p>Treasure is %s %s</p>\n",
           tech_level_names[items[treasure].efficiency],
           item_names[items[treasure].sort]);
  // FIXME
  player->ship = transfer_item (treasure, player);
  if (flags & BONUS_DIVINE)
    {
      fprintf (fd,
               "<p>%s smiles on this adventure and gives %d extra favour</p>\n",
               god_names[skill], level * 5 + 3);
      player->favour[skill] += level * 5 + 3;
      add_favour (player, skill, level * 5 + 3);
    }
  if ((flags & BONUS_SCIENCE) && (player->away_team[science] == parameter))
    {
      fprintf (fd, "<p>Science crew spot clues:</p>\n");
      generate_rumour (fd, player, 0);
      for (i = 0; i < 4; i++)
        if (dice (100) < effective_skill_level (player, science))
          generate_rumour (fd, player, 0);
    }
  fprintf (fd, "</aside>\n");
  adventures[adventure].treasure = 0;   /* means regenerate */
  reset_adventure (adventure);
  player->skills[skill] |=
    skill_bit (adventure_skill, ADVENTURE_TYPE (parameter));
  return (TRUE);
}

void
init_adventures ()
{
  int i;

  for (i = 0; i < MAX_ADVENTURE; i++)
    {
      adventures[i].treasure = 0;
    }
  reset_adventures ();
}


void
find_adventure (FILE * fd, struct PLAYER *player)
{
  int ad, i;

  i = 0;
  do
    {
      ad = dice (MAX_ADVENTURE);
      if (i++ > 1000)
        break;
    }
  while ((ADVENTURE_LEVEL (ad) >
         skill_level (player->skills[ADVENTURE_SKILL (ad)]))
         || get_ad (player, ad));
  set_ad (player, ad);
  fprintf (fd, "<li>The Wise One reveals an adventure at %s:</li>\n",
           star_names[adventures[ad].star]);
  show_adventure (fd, ad);
}

void
update_adventures (struct PLAYER *player)
{
  int ad;

  for (ad = 0; ad < MAX_ADVENTURE; ad++)
    {
      adventures[ad].star = locations[adventures[ad].loc].star;
      if (adventures[ad].star == OLYMPUS &&
          get_ad (player, ad) && player->chosen < OLYMPUS_SEEN)
        {
          printf ("Hiding %d from %s\n", ad, player->name);
          reset_ad (player, ad);
        }
      if (adventures[ad].star == player->star)
        {
          if (adventures[ad].obscurity <= factor (sensor, player))
            set_ad (player, ad);
          if (player->
              passwd_flags & (8 <<
                              (4 *
                               ADVENTURE_SKILL (adventures[ad].parameter))))
            {
              set_ad (player, ad);
            }
        }
    }
}

void
set_ad (struct PLAYER *player, int ad)
{
  set_bit(player->ads,ad);
}

void
reset_ad (struct PLAYER *player, int ad)
{
  reset_bit(player->ads,ad);
}

int
get_ad (struct PLAYER *player, int ad)
{
  return get_bit(player->ads, ad);
}



