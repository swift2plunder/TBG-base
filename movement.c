#include <math.h>
#include "movement.h"
#include "globals.h"
#include "tbg.h"
#include "rand.h"
#include "util.h"
#include "tbg-big.h"
#include "items.h"

int
square_distance (int s1, int s2)
{
  int xdiff, ydiff;

  if (s1 > MAX_STAR || s2 > MAX_STAR)
    {
      printf ("square distance called for holiday/invalid star: %d %d\n",
              s1, s2);
      return 0;
    }

  xdiff = stars[s1].x - stars[s2].x;
  ydiff = stars[s1].y - stars[s2].y;
  return (xdiff * xdiff + ydiff * ydiff);
}

int
distance (int s1, int s2)
{
  return (isqrt (square_distance (s1, s2)));
}

void
show_destinations (FILE * fd, int self, int current,
                   double min, double max, int showcost)
{
  int star, loc, player;
  double min2 = min*min;
  double max2 = max*max;
  struct PLAYER *p = players + self;

  fprintf (fd, "<select name=\"j\">\n");
  fprintf (fd, "<option value=\" \">No Jump</option>\n");

  if (p->star == HOLIDAY || p->star >= MAX_STAR)
    {
      fprintf (fd, "<option value=\"%2d\">%s %c ($0)</option>\n",
               p->old_star, star_names[p->old_star],
               terrain_names[stars[p->old_star].terrain][0]);
      fprintf (fd, "</select>\n");
      return;
    }

  if (min2 <= 0.0)
    {
      /* holiday options */
      
      if (p != dybuk)
        fprintf (fd, "<option value=\"%d\">Start Holiday ($0)</option>\n", MAX_STAR + self);
      
      /* chasings */
      for (player = 0; player < MAX_PLAYER; player++)
        if (player != self && players[player].star == current)
          fprintf (fd, "<option value=\"%d\">Chase %s</option>\n",
                   player + BIG_NUMBER, name_string (players[player].name));
    }
  if (factor (warp_drive, p) > 0  || !showcost)
    {
      /* conventional jumps */
      for (star = -2; star < MAX_STAR ; star++)
        {
          int r2;
          if (star == HOLIDAY || star >= MAX_STAR)
            continue;
          /* Olympus Dyson Sphere */
          if (star == OLYMPUS && p->chosen <= OLYMPUS_SEEN)
            /* ie seen it AND chosen */
            continue;
          r2 = square_distance (star, current);
          if (current != star && (r2 >= min2)  && (r2 <= max2)
              && (get_bit(p->stars,star)
                  || get_bit (public_stars, star)))
            {
              if (showcost)
                {
                  fprintf (fd, "<option value=\"%d\">%s %c ($%d)</option>\n",
                           star, star_names[star],
                           terrain_names[stars[star].terrain][0],
                           jump_cost(self, current, star));
                }
              else
                {
                  fprintf (fd, "<option value=\"%d\">%s %c</option>\n",
                           star, star_names[star],
                           terrain_names[stars[star].terrain][0]);
                }
            }
        }
    }
  /* stargates, use loc as "star", secure because they start at 1300 */
  if (min2 <= 0.0)
    {
      for (loc = 0; loc < MAX_LOCATION; loc++)
        if ((locations[loc].star == current) && (locations[loc].sort == stargate))
          fprintf (fd, "<option value=\"%d\">%s (Gate)</option>\n",
                   loc, star_names[locations[loc].parameter]);
    }
  fprintf (fd, "</select>\n");
}

int
star_seen (struct PLAYER *p, int star)
{
  if (p == dybuk)
    return (star >= 0 && star < MAX_STAR && get_bit (evil_stars, star));
  else if (star == OLYMPUS)
    return p->chosen <= OLYMPUS_SEEN;
  else if (star < 0 || star >= MAX_STAR)
    return 0;
  else
    return (get_bit(p->stars, star) || get_bit(public_stars, star));
}


int
jump_cost (int ship, int s1, int s2)
{
  double r = sqrt(square_distance(s1,s2));
  double w = factor (warp_drive, players + ship);
  double c = 2000000000;
  if (w > 0)
    c = exp((4.0 * r)/log(w))/200.0;
  return (c > 2000000000) ? 2000000000 : (int) c;
}

double
inverse_jump_cost (int ship, int c)
{
  double w = factor (warp_drive, players + ship);
  if (w > 0)
    return (log(c/180.0) + 10.5) * log(w) / 4.0;
  else
    return 0;
}


int
get_random_star (struct PLAYER *player)
{
  int s;
  int n = 0;
  for (s = 0 ; s < MAX_STAR ; s++)
    {
      if (star_seen (player, s))
        n++;
    }
  if (n > 0)
    {
      n = dice(n);
      for (s = 0 ; s < MAX_STAR ; s++)
        {
          if (star_seen (player, s) && !n--)
            break;
        }
    }
  else
    {
      s = dice (MAX_STAR);
      set_bit (player->stars, s);
    }
  return s;
}

  
void
jump (FILE * fd, struct PLAYER *player, int from, int to)
{
  int cost;

  if (!player->got_some_orders) /* no move */
    {
      if (to > MAX_STAR)
        {
          fprintf (fd,
                   "<p>Missed several turns, automatically jumped to %s Planet</p>\n",
                   (mothballed (player - players)) ? "Mothball" : "Holiday");
          if (from < MAX_STAR)
            player->old_star = from;
          player->star = to;
        }
      return;
    }
  if (from == HOLIDAY || from >= MAX_STAR)
    if (to != player->old_star && to != -1)
      {
        printf ("%s tried for %d, should be %d\n",
                player->name, to, player->old_star);
        return;
      }
  if (to == from)               /* no move */
    return;
  if (to == HOLIDAY || (to >= MAX_STAR && to < MAX_STAR + MAX_PLAYER))  /* going on holiday */
    {
      fprintf (fd, "<p>Jumped to Holiday Planet</p>\n");
      player->old_star = from;
      player->star = MAX_STAR + player - players;
      printf ("%s went on holiday\n", player->name);
      return;
    }
  if (from == HOLIDAY || from >= MAX_STAR)      /* returning from holiday */
    {
      if (! star_seen (player, player->old_star))
        {
          player->old_star = get_random_star (player);
          fprintf (fd, "<p>Massive energy wave sends you to %s</p>\n",
                   star_names[player->old_star]);
        }
      else
        {
          fprintf (fd, "<p>Jumped to %s for $0</p>\n", star_names[player->old_star]);
        }
      if (player->star >= 0 && player->star < MAX_STAR)
          set_bit(player->stars, player->star);
      player->star = player->old_star;
      if (player->star >= 0 && player->star < MAX_STAR)
          set_bit(player->stars, player->star);
      return;
    }
  if (from == NOWHERE)
    {
      fprintf (fd, "<p>Massive energy wave sends you to %s</p>\n",
               star_names[to]);
      player->star = to;
      player->old_star = to;
      return;
    }
  if (to > MAX_STAR + MAX_PLAYER
      && any_gates (player, from, locations[to].parameter))
    {
      cost = 0;
      to = locations[to].parameter;
    }
  else
    {
      if (to > MAX_STAR + MAX_PLAYER)
        {
          printf ("%s tries to move to %d\n", player->name, to);
          fprintf (fd, "<p>No key to use that stargate</p>\n");
          return;
        }
      if (factor (warp_drive, player) == 0)
        {
          fprintf (fd, "<p>No warp engines, No jump</p>\n");
          return;
        }
      cost = jump_cost (player - players, from, to);
      if (player->energy < cost)
        {
          fprintf (fd, "<p>Not enough energy to jump, need $%d</p>\n",
                   cost);
          return;
        }
      player->energy -= cost;
    }
  if (player->star >= 0 && player->star < MAX_STAR)
      set_bit(player->stars, player->star);
  player->old_star = player->star;
  player->star = to;
  if (player->star >= 0 && player->star < MAX_STAR)
    set_bit(player->stars, player->star);  
  if (to < MAX_STAR)
    {
      fprintf (fd, "<p>Jumped to %s for $%d</p>\n", star_names[to], cost);
    }
  else
    {
      fprintf (fd, "<p>Jumped to Holiday Planet for %s for $%d</p>\n",
               name_string (player->name), cost);
    }
}

