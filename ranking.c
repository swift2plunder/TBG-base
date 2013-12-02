#include <stdio.h>
#include "globals.h"
#include "ranking.h"
#include "tbg.h"
#include <stdlib.h>
#include "defs.h"
#include "util.h"
#include <string.h>
#include "skill.h"
#include "tbg-big.h"
#include "items.h"
#include "combat.h"

#define SYSTEM(command) { system(command); }

int
old_rank (int p)
{
  int rating;
  struct ITEM *item;
  struct PLAYER *player = players + p;

  if (turn - player->last_restart < 1)
    return (0);
  if (strcmp (player->address, "tbg-moderator@asciiking.com") == 0)
    return (0);
  rating = player->energy - 500
    + 10 * player->torps
    + 100 *
    (effective_skill_level (player, engineering) +
     effective_skill_level (player, science) +
     effective_skill_level (player, medical) +
     effective_skill_level (player, weaponry));
  item = items + player->ship;
  while (item != items)
    {
      if (!(item->flags & ITEM_BROKEN))
        rating += sale_price (item) * 2;
      if (item->sort == pod && item->reliability &&
          item->reliability < BASE_UNIT)
        rating += item->collection * goods[item->reliability].basic_value;
      item = items + item->link;
    }
  if (rating < 0)
    rating = 0;
  return (rating / (turn - player->last_restart));
}


int
rank_engineering_skill (int player)
{
  return (skill_level (players[player].skills[engineering]));
}

int
rank_science_skill (int player)
{
  return (skill_level (players[player].skills[science]));
}

int
rank_medical_skill (int player)
{
  return (skill_level (players[player].skills[medical]));
}

int
rank_weaponry_skill (int player)
{
  return (skill_level (players[player].skills[weaponry]));
}

int
rank_total_skill (int player)
{
  return (effective_skill_level (players + player, engineering) +
          effective_skill_level (players + player, science) +
          effective_skill_level (players + player, medical) +
          effective_skill_level (players + player, weaponry));
}

int
rank_total_engineering_skill (int player)
{
  return (effective_skill_level (players + player, engineering));
}

int
rank_total_science_skill (int player)
{
  return (effective_skill_level (players + player, science));
}

int
rank_total_medical_skill (int player)
{
  return (effective_skill_level (players + player, medical));
}

int
rank_total_weaponry_skill (int player)
{
  return (effective_skill_level (players + player, weaponry));
}

int
rank_total_officer_skill (int player)
{
  return (skill_level (players[player].skills[engineering]) +
          skill_level (players[player].skills[science]) +
          skill_level (players[player].skills[medical]) +
          skill_level (players[player].skills[weaponry]));
}

int
rank_power (int player)
{
  return (power (players + player));
}

int
rank_modules (int player)
{
  struct ITEM *it = items + players[player].ship;
  int size = 0;

  while (it != items)
    {
      size++;
      it = items + it->link;
    }
  return (size);
}

int
rank_growth (int player)
{
  if (players[player].restarting)
    return (0);
  return (((rank_modules (player) - 9) * 100) / (turn -
                                                 players[player].
                                                 last_restart));
}

int
rank_reliability (int player)
{
  struct ITEM *item = items + players[player].ship;
  int result = 0, total = 0;

  while (item != items)
    {
      if (item->sort < pod)
        {
          result += item->reliability;
          total++;
        }
      item = items + item->link;
    }
  if (total)
    return (result / total);
  else
    return (0);
}

int
rank_weapons (int player)
{
  struct ITEM *item = items + players[player].ship;
  int result = 0;

  while (item != items)
    {
      if (item->sort >= shield && item->sort <= fighter)
        result += item->efficiency;
      item = items + item->link;
    }
  return (result);
}

int
rank_torps (int player)
{
  return (players[player].torps);
}

int
rank_money (int player)
{
  return (players[player].energy);
}

int
rank_wealth (int player)
{
  struct ITEM *item = items + players[player].ship;
  int result = players[player].energy;

  while (item != items)
    {
      if (item->sort == pod && item->reliability &&
          item->reliability < BASE_UNIT)
        result += item->collection * goods[item->reliability].basic_value;
      item = items + item->link;
    }
  return (result);
}

int
rank_ship_value (int player)
{
  struct ITEM *item = items + players[player].ship;
  int result = 0;

  while (item != items)
    {
      result += sale_price (item);
      item = items + item->link;
    }
  return (result);
}

int
rank_artifacts (int player)
{
  struct ITEM *item = items + players[player].ship;
  int result = 0;

  while (item != items)
    {
      if (item->sort == artifact)
        result++;
      item = items + item->link;
    }
  return (result);
}

int
rank_pods (int player)
{
  struct ITEM *item = items + players[player].ship;
  int result = 0;

  while (item != items)
    {
      if (item->sort == pod)
        result += item->efficiency;
      item = items + item->link;
    }
  return (result);
}

int
rank_playing_since (int player)
{
  return (turn - players[player].last_restart);
}

int
rank_newest (int player)
{
  if (players[player].restarting)
    return (0);
  else
    return (players[player].last_restart);
}

int
rank_engineering_favour (int player)
{
  if (players[player].favour[engineering] < 0)
    return (0);
  return (players[player].favour[engineering]);
}

int
rank_science_favour (int player)
{
  return (players[player].favour[science]);
}

int
rank_medical_favour (int player)
{
  return (players[player].favour[medical]);
}

int
rank_weaponry_favour (int player)
{
  return (players[player].favour[weaponry]);
}

int
rank_total_favour (int player)
{
  return (players[player].favour[engineering] +
          players[player].favour[science] +
          players[player].favour[medical] + players[player].favour[weaponry]);
}

int
rank_total_crew (int player)
{
  return (players[player].crew[engineering] +
          players[player].crew[science] +
          players[player].crew[medical] + players[player].crew[weaponry]);
}

int
rank_health (int player)
{
  if (rank_total_crew (player))
    return (players[player].health);
  else
    return (0);
}

int
rank_total_adventures (int player)
{
  int i, result = 0;

  for (i = 0; i < MAX_ADVENTURE / 32; i++)
    result += bitcount (players[player].ads[i]);
  return (result);
}

int
rank_total_enemies (int player)
{
  int result = 0, i;

  for (i = 0; i < 32; i++)
    if (players[player].enemies & (1 << i))
      result++;
  return (result);
}

int
rank_total_cures (int player)
{
  int result = 0, i;

  for (i = 0; i < 32; i++)
    if (players[player].experience[medical] & (1 << i))
      result++;
  return (result);
}

int
rank_total_terminals (int player)
{
  int result = 0, i;

  for (i = 0; i < 32; i++)
    if (players[player].experience[science] & (1 << i))
      result++;
  return (result);
}

int
rank_warp_factor (int player)
{
  return (factor (warp_drive, players + player));
}

int
rank_impulse_factor (int player)
{
  return (factor (impulse_drive, players + player));
}

int
rank_sensor_factor (int player)
{
  return (factor (sensor, players + player));
}

int
rank_cloak_factor (int player)
{
  return (factor (cloak, players + player));
}

int
rank_sickbay_factor (int player)
{
  return (factor (sick_bay, players + player));
}

int
rank_life_factor (int player)
{
  return (factor (life_support, players + player));
}

int
rank_shield_factor (int player)
{
  return (factor (shield, players + player));
}

int
rank_weapon_factor (int player)
{
  return (factor (ram, players + player));
}

int
rank_votes (int player)
{
  return (players[player].votes);
}

int
rank_plebs (int player)
{
  if (turn - players[player].last_restart > 100)
    return (-1);
  else
    return (players[player].plebs);
}

int
rank_pollution (int player)
{
  return (players[player].pollution);
}

#define MAX_RANKING 44
struct RANKING
{
  int (*funptr) (int player);
  char *string;
}
  rankings[MAX_RANKING] =
    {
      rank_playing_since, "Longest Playing",
      rank_newest, "Newest",
      rank_votes, "Most Presidential Votes",
      rank_plebs, "Most Tribune Votes",
      rank_engineering_skill, "Best Engineering Officer Skill",
      rank_science_skill, "Best Science Officer Skill",
      rank_medical_skill, "Best Medical Officer Skill",
      rank_weaponry_skill, "Best Weaponry Officer Skill",
      rank_total_officer_skill, "Best Total Officer Skill",
      rank_total_engineering_skill, "Highest Total Engineering Skill",
      rank_total_science_skill, "Highest Total Science Skill",
      rank_total_medical_skill, "Highest Total Medical Skill",
      rank_total_weaponry_skill, "Highest Total Weaponry Skill",
      rank_total_skill, "Highest Total Skill",
      rank_total_crew, "Most Total Crew",
      rank_health, "Highest Crew Health",
      rank_reliability, "Highest Average Reliability",
      rank_power, "Biggest Ship",
      rank_modules, "Most Modules",
      rank_growth, "Growth in Modules",
      rank_weapons, "Best Armed Ship",
      rank_torps, "Most Torpedoes",
      rank_money, "Richest Player in Cash Only",
      rank_wealth, "Richest Player in Trade Goods & Cash",
      rank_ship_value, "Most Valuable Ship",
      rank_pods, "Most Pod Space",
      rank_artifacts, "Most Artifacts",
      rank_engineering_favour, "Highest Engineering Favour",
      rank_science_favour, "Highest Science Favour",
      rank_medical_favour, "Highest Medical Favour",
      rank_weaponry_favour, "Highest Weaponry Favour",
      rank_total_favour, "Highest Total Favour",
      rank_total_adventures, "Most Known Adventures",
      rank_total_enemies, "Most Enemies",
      rank_total_cures, "Most Plagues Cured",
      rank_total_terminals, "Most Starnet Accesses",
      rank_warp_factor, "Highest Warp Drive %",
      rank_impulse_factor, "Highest Impulse Drive %",
      rank_sensor_factor, "Highest Sensor %",
      rank_cloak_factor, "Highest Cloak %",
      rank_sickbay_factor, "Highest Sick Bay %",
      rank_life_factor, "Highest Life Support %",
      rank_shield_factor, "Highest Shield %",
      rank_weapon_factor, "Highest Weapon %",
      };

void
do_rankings (FILE * fd)
{
  int best, high_score, second_best, second_score;
  int i, player, score, total, num_players;

  fprintf (fd, "<hr><table class=\"rankings\">\n");
  fprintf (fd, "<tr><th colspan=\"4\">Rankings</th></tr>\n");
  fprintf (fd, "<tr><th>Category</th><th>First</th><th>Second</th>");
  fprintf (fd, "<th>Average</th></tr>\n");
  for (i = 0; i < MAX_RANKING; i++)
    {
      num_players = 0;
      best = 0;
      high_score = 0;
      second_best = 0;
      second_score = 0;
      total = MAX_PLAYER / 2;
      for (player = 0; player < MAX_PLAYER; player++)
        {
          if (strcmp (players[player].address, "tbg-moderator@asciiking.com") == 0
              || ! players[player].account_number)
            continue;
          if (mothballed(player))
            continue;
          num_players++;
          score = rankings[i].funptr (player);
          total += score;
          if (score > high_score ||
              (score == high_score
               && players[player].last_restart >= players[best].last_restart))
            {
              second_best = best;
              second_score = high_score;
              high_score = score;
              best = player;
            }
          else
            if (score > second_score ||
                (score == second_score
                 && players[player].last_restart >=
                 players[second_best].last_restart))
              {
                second_best = player;
                second_score = score;
              }
        }
      if (really_send)
        fprintf (fd,
                 "<tr><td><a href=\"http://%s/rank%d.html\">%s</a></td><td>%s</td>\n",
                 server, i, rankings[i].string,
                 name_string (players[best].name));
      else
        fprintf (fd,
                 "<tr><td><a href=\"http://%s/tbg/fullrank%d.html\">%s</a></td><td>%s</td>\n",
                 server, i, rankings[i].string,
                 name_string (players[best].name));
      fprintf (fd, "<td>%s</td><td>%d</td></tr>\n",
               name_string (players[second_best].name),
               total / (num_players ? num_players : 1));
    }
  fprintf (fd, "</table>\n");
}




void
big_rank (FILE * fd, struct PLAYER *player, int index)
{
  int rating;

  if (index == -1)
    rating = old_rank (player - players);
  else
    rating = rankings[index].funptr (player - players);
  fprintf (fd, "%12d %5d %4d\n", rating, player->last_restart,
           player - players);
}

int
big_ranking (FILE * output, int index, char *title)
{
  struct PLAYER *player;
  int rating, p, i, start;
  int n_selected;
  FILE *fd;
  char buffer[256];
  int rating_off = ((index != -1) && (really_send));

  sprintf (buffer, "%s/ranks", desired_directory);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create ranking file\n");
      exit (1);
    }
  for (player = players + 1; player < players + MAX_PLAYER; player++)
    {
      if ((strcmp (player->address, "nobody@localhost") == 0)
          || (player->account_number == 0))
        continue;
      if (mothballed(player - players))
        continue;    
      big_rank (fd, player, index);
    }
  fclose (fd);
  sprintf (buffer, "/bin/sort -r -g %s/ranks >%s/sorted",
           desired_directory, desired_directory);
  SYSTEM (buffer);
  sprintf (buffer, "%s/sorted", desired_directory);
  fd = fopen (buffer, "r");
  if (!fd)
    {
      printf ("Can't open sorted file\n");
      exit (1);
    }
  fprintf (output,
           "<table class=\"rankings\"><tr><th colspan=\"%d\">\n%s\n</th></tr>\n",
           rating_off ? 3 : 4, title);
  fprintf (output, "<tr><th>Rank</th><th>Ship</th>\n");
  if (!rating_off)
    fprintf (output, "<th>Rating</th>\n");
  fprintf (output, "<th>Started</th></tr>\n");
  n_selected = 0;
  for (i = 1; i < MAX_PLAYER; i++)
    {
      fscanf (fd, "%d %d %d\n", &rating, &start, &p);
      if (index == -1)
        players[p].ranking = i;
      // EEM
      /* filter is redundant
      if ((strcmp (players[p].address, "nobody@localhost") == 0)
                 || (player->account_number == 0)) 
      */
      //if (player->account_number == 0) //Alternative to above
      //  continue;
      if (n_selected++ >= 20)
        continue;
      if (rating_off)
        fprintf (output,
                 "<tr><td>%d</td><td>%s</td><td>%d</td></tr>\n",
                 n_selected, name_string (players[p].name),
                 players[p].last_restart);
      else
        fprintf (output,
                 "<tr><td>%d</td><td>%s</td><td>%d</td><td>%d</td></tr>\n",
                 n_selected, name_string (players[p].name), rating,
                 players[p].last_restart);
    }
  fprintf (output, "</table>\n");
  fclose (fd);
}

void
all_rankings ()
{
  int i;
  FILE *fd;
  char buffer[256];

  for (i = 0; i < MAX_RANKING; i++)
    {
      if (really_send)
        sprintf (buffer, "%s/rank%d.html", webroot, i);
      else
        sprintf (buffer, "%s/fullrank%d.html", webroot, i);
      fd = fopen (buffer, "w");
      if (!fd)
        {
          printf ("Can't open ranking %d file\n", i);
          exit (1);
        }
      big_ranking (fd, i, rankings[i].string);
      fclose (fd);
    }
}

