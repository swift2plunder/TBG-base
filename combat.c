#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include "defs.h"
#include "globals.h"
#include "combat.h"
#include "tbg.h"
#include "util.h"
#include "items.h"
#include "locations.h"
#include "religion.h"
#include "rand.h"
#include "tbg-big.h"
#include "orders.h"
#include "skill.h"

#define SHIELD_POWER 360

int *loot_array;
int loot_max;

void
destroy_loot ()
{
  int i;
  for (i = 0 ; i < loot_max ; i++)
    {
      int item = loot_array[i];
      destroy_item (item);
    }  
}
  

void
save_loot (int item, int previous_owner)
{
  int curse, bad_bits;

  if (item == NO_ITEM)
    return;

  items[item].pob = previous_owner;

  if (items[item].sort == evil_artifact)
    {
      destroy_item(item);
      return;
    }

  if (items[item].sort == artifact)
    {
      /* curse on transfer */
      bad_bits = items[item].magic & 0xff0000;      /* blessings */
      bad_bits |= (items[item].magic & 0xff00) << 8;        /* curses */

      if (bad_bits != 0xff0000)
        {
          do
            curse = dice (8);
          while (bad_bits & (0x10000 << curse));
          items[item].magic |= 0x100 << curse;
        }
    }
  remove_item (item);           /* remove from old */
  loot_array[loot_max++] = item;
}

int 
split_loot (FILE *fd, struct PLAYER *winner, struct PLAYER *loser)
{
  int total_damage, real_total;
  int i, j;
  int factor = 1000;
  int cost = 0;
  int max_tech = 0;
  
  if (! loot_max)
    return 0;

  for (i = 0 ; i < loot_max ; i++)
    {
      int tech;
      int item = loot_array[i];

      if (items[item].sort == artifact)
        tech = 4;
      else
        tech = items[item].efficiency;
      max_tech = max (max_tech, tech);
    }

  winner->damage += max_tech;
  loser->damage += max_tech;

  real_total = winner->damage + loser->damage;
  
  winner->damage *= 2;
  total_damage = 1 + winner->damage + loser->damage;

  winner->freebies = winner->damage -
    ((winner->damage - max_tech) * real_total)/total_damage;
  loser->freebies = loser->damage -
    ((loser->damage - max_tech) * real_total)/total_damage;

  /* damage items in order shot off */
  for (i = 0 ; i < loot_max; i++)
    {
      int item = loot_array[i];
      int r;
      if (items[item].sort < pod)
        {
          factor *= 1000 - rand_exp (50);
          factor /= 1000;
          r = factor * items[item].reliability;
          r /= 1000;
          if (r <= 0)
            r = 0;
          cost += (items[item].reliability - r)
            * (1 << items[item].efficiency) / 4;
          items[item].reliability = r;
          items[item].flags |= ITEM_BROKEN;
        }
    }

  /* randomize order of loot array */
  for (i = 0 ; i < loot_max - 1; i++)
    {
      int j = i + dice(loot_max - i);
      int temp = loot_array[j];
      loot_array[j] = loot_array[i];
      loot_array[i] = temp;
    }

  /* award loot in random order */
  for (i = 0 ; i < loot_max ; i++)
    {
      int item = loot_array[i];
      int tech, r;
      int flags = items[item].flags;
      struct PLAYER *defender = ships + items[item].pob;
      struct PLAYER *attacker = (defender == winner) ? loser : winner;
      struct PLAYER *looter;

      if (items[item].sort == artifact)
        tech = 4;
      else if (items[item].sort == evil_artifact)
        tech = 0; /* evil artifacts *choose* where to go */
      else
        tech = items[item].efficiency;

      r = items[item].reliability;
      if (items[item].sort < pod)
        {
          if (r <= 0)
            {
              destroy_item (item);
              continue;
            }
        }
      if (items[item].sort == evil_artifact)
        {
          if (winner == dybuk)
            looter = winner;
          else if (loser == dybuk)
            looter = loser;
          else if (winner->evil >= loser->evil)
            looter = winner;
          else
            looter = loser;
        }
      else if (loser->ship == 0)
        looter = winner;
      else if (flags & ITEM_PROTECTED
               && !(flags & ITEM_TARGETTED)
               && defender->damage > tech
               && defender->damage > defender->freebies)
        looter = defender;
      else if (flags & ITEM_TARGETTED
               && !(flags & ITEM_PROTECTED)
               && attacker->damage > tech
               && attacker->damage > attacker->freebies)
        looter = attacker;
      else if (flags & ITEM_DEMANDED
               && (attacker == winner
                   || attacker->damage > defender->damage)
               && attacker->damage > tech)
        looter = attacker;
      else if (defender->damage > defender->freebies) 
        looter = defender;
      else if (dice(total_damage) < winner->damage)
        looter = winner;
      else
        looter = loser;

      items[item].flags &= ITEM_SAVE_FLAGS;
      
      looter->ship = add_item (looter, item);
      looter->damage -= tech;
      total_damage -= tech;
      if (items[item].sort < pod)
        fprintf (fd , "%s gains %s %s (%d\%) as loot.<BR>\n",
                 name_string(looter->name),
                 tech_level_names[tech],
                 item_string(items+item), r);
      else
        fprintf (fd , "%s gains %s as loot.<BR>\n",
                 name_string(looter->name),
                 item_string(items+item));
    }
  return cost;
}

struct PLAYER *
link_ship (struct PLAYER *base, struct PLAYER *new)
{
  int new_power = power (new);
  struct PLAYER *link = base;

  if (power (link) < new_power) /* head of list special case */
    {
      new->next = link;
      return (new);
    }

  while (link->next && (power (link->next) > new_power))
    link = link->next;
  
  new->next = link->next;
  link->next = new;
  return (base);
}

struct PLAYER *
sort_ships (int star)
{
  int p, rank = 1;
  struct PLAYER *base = 0, *next;

  if (star >= MAX_STAR)
    {
      int p = star - MAX_STAR;
      if (players[p].star == star)
        {
          base = players + p;
          base->next = 0;
        }
      return base;
    }


  for (p = 0; p < MAX_PLAYER + MAX_ALIEN; p++)
    if (players[p].star == star)
      {
        if (escorted (players + p))
          continue;
        if (base == 0)
          {
            base = players + p;
            players[p].next = 0;
          }
        else
          base = link_ship (base, players + p);
      }
  next = base;
  while (next)
    {
      next->rank = rank++;
      next = next->next;
    }
  for (p = 0; p < MAX_PLAYER + MAX_ALIEN; p++)
    if (escorted (players + p))
      players[p].rank = players[players[p].companion].rank;
  return (base);
}


/* returns ship paired with player for interaction */
struct PLAYER *
pairing (struct PLAYER *player)
{
  struct PLAYER *base;

  base = sort_ships (player->star);
  while (TRUE)
    {
      if (!base)
        return (0);
      if (base == player)
        return (base->next);
      if (base->next == player)
        return (base);
      if (!base->next)
        return (0);
      base = base->next->next;
    }
}

void
show_combat_options (FILE * fd, struct PLAYER *player, struct PLAYER *enemy)
{
  struct ITEM *item;
  int i, loc, range, num_items = 0;
  char buffer[1024];

  if (enemy - players < MAX_PLAYER)
    snprintf (buffer, 1024,
              "<a href=http://%s/tbg/news/mail.cgi?%d>%s</a>",
             server, enemy - players, name_string (enemy->name));
  else
    strcpy (buffer, name_string (enemy->name));

  fprintf (fd, "<TABLE BORDER=1>\n");
  fprintf (fd,
           "<TR><TH COLSPAN=2 ALIGN=CENTER>Interaction with %s</TH></TR>\n",
           buffer);

  item = items + player->ship;
  while (item - items)
    {
      num_items++;              /* used for retreat threshold below */
      item = items + item->link;
    }

  fprintf (fd, "<TR ALIGN=CENTER><TH>Diplomatic Option</TH>\n");
  fprintf (fd, "<TH>Demands</TH></TR>\n");

  fprintf (fd, "<TR ALIGN=CENTER><TD><SELECT NAME=\"do\">\n");
  fprintf (fd, "<OPTION VALUE=0>Flee\n");
  fprintf (fd, "<OPTION VALUE=1>Fight if Attacked\n");
  fprintf (fd, "<OPTION VALUE=2>Make Demands\n");
  fprintf (fd, "<OPTION VALUE=3>Attack If Defied\n");
  fprintf (fd, "<OPTION VALUE=4>Attack Regardless\n");
  fprintf (fd, "</SELECT></TD>\n");

  fprintf (fd, "<TD><SELECT NAME=\"dd\" SIZE=5>\n");
  fprintf (fd, "<OPTION VALUE=\"\">None\n");
  item = items + enemy->ship;
  while (item - items)
    {
      fprintf (fd, "<OPTION VALUE=%d>%s\n", item - items, item_string (item));
      item = items + item->link;
    }
  fprintf (fd, "</SELECT></TD></TR>\n");

  fprintf (fd, "<TR ALIGN=CENTER><TH>Combat Strategy</TH>\n");
  fprintf (fd, "<TH>Gifts</TH></TR>\n");

  fprintf (fd, "<TR ALIGN=CENTER><TD><SELECT NAME=\"dc\">\n");
  fprintf (fd, "<OPTION VALUE=0>Favour Fleeing\n");
  fprintf (fd, "<OPTION VALUE=1>Favour Engines\n");
  fprintf (fd, "<OPTION VALUE=2>Favour Weapons\n");
  fprintf (fd, "<OPTION VALUE=3>Favour Shields\n");
  fprintf (fd, "<OPTION VALUE=4>Favour Sensors\n");
  fprintf (fd, "<OPTION VALUE=5>Favour Cloaks\n");
  fprintf (fd, "</SELECT></TD>\n");

  fprintf (fd, "<TD><SELECT NAME=\"dg\" SIZE=5 MULTIPLE>\n");
  fprintf (fd, "<OPTION VALUE=\"\">None\n");
  fprintf (fd, "<OPTION VALUE=-1>Any one module\n");
  item = items + player->ship;
  while (item - items)
    {
      fprintf (fd, "<OPTION VALUE=%d>%s\n", item - items, item_string (item));
      item = items + item->link;
    }
  fprintf (fd, "</SELECT></TD></TR>\n");

  fprintf (fd, "<TR ALIGN=CENTER><TH>Ideal Range</TH>\n");
  fprintf (fd, "<TH>Targetted</TH></TR>\n");

  fprintf (fd, "<TR ALIGN=CENTER><TD><SELECT NAME=\"di\">\n");
  for (range = 0; range < 7; range++)
    fprintf (fd, "<OPTION VALUE=%d %s>%s\n",
             range,
             range == find_longest_weapon (items + player->ship) ? "SELECTED" : "",
             range_names[range]);
  fprintf (fd, "</SELECT></TD>\n");

  fprintf (fd, "<TD><SELECT NAME=\"dt\" SIZE=5 MULTIPLE>\n");
  fprintf (fd, "<OPTION VALUE=\"\">None\n");
  item = items + enemy->ship;
  while (item - items)
    {
      fprintf (fd, "<OPTION VALUE=%d>%s\n", item - items, item_string (item));
      item = items + item->link;
    }
  fprintf (fd, "</SELECT></TD></TR>\n");

  fprintf (fd, "<TR ALIGN=CENTER><TH>Retreat Threshold</TH>\n");
  fprintf (fd, "<TH>Protected</TH></TR>\n");

  fprintf (fd, "<TR ALIGN=CENTER><TD><SELECT NAME=\"dr\">\n");
  for (i = 0; i < num_items; i++)
    fprintf (fd, "<OPTION VALUE=%d %s>%d\n", i, i == 1 ? "selected" : "", i);
  fprintf (fd, "</SELECT></TD>\n");

  fprintf (fd, "<TD><SELECT NAME=\"dp\" SIZE=5 MULTIPLE>\n");
  fprintf (fd, "<OPTION VALUE=\"\">None\n");
  item = items + player->ship;
  while (item - items)
    {
      fprintf (fd, "<OPTION VALUE=%d>%s\n", item - items, item_string (item));
      item = items + item->link;
    }
  fprintf (fd, "</SELECT></TD></TR>\n");

  fprintf (fd, "<TR ALIGN=CENTER><TH>Torpedo Fire Rate</TH>\n");
  fprintf (fd, "<TH>Hunt/Hide Tactics</TH></TR>\n");

  num_items = isqrt (player->torps);
  fprintf (fd, "<TR ALIGN=CENTER><TD><SELECT NAME=\"df\">\n");
  for (i = 0; i <= num_items; i++)
    fprintf (fd, "<OPTION VALUE=%d>%d per round\n", i, i * i);
  fprintf (fd, "</SELECT>\n");
  fprintf (fd, "</TD>\n");

  fprintf (fd, "<TD><SELECT NAME=\"dh\" SIZE=5>\n");
  fprintf (fd, "<OPTION VALUE=\"\">None\n");
  fprintf (fd, "<OPTION VALUE=1>Hide in Space\n");
  fprintf (fd, "<OPTION VALUE=-1>Hunt in Space\n");
  for (loc = 0; loc < MAX_LOCATION; loc++)
    if (locations[loc].star == player->star && loc_type (loc, LOC_HIDE))
      {
        fprintf (fd, "<OPTION VALUE=%d>Hide in %s (%d%%)\n",
                 loc, loc_string (loc), risk_level (player, loc));
        fprintf (fd, "<OPTION VALUE=-%d>Hunt in %s (%d%%)\n",
                 loc, loc_string (loc), risk_level (player, loc));
      }
  fprintf (fd, "</SELECT></TD></TR>\n");
  fprintf (fd, "</TABLE>");
}

double
average_shields (struct PLAYER *defender)
{
  struct ITEM *it = items + defender->ship;
  double num_items = 0.0, shields = 0.0;
  while (it != items)
    {
      if (!(it->flags & ITEM_BROKEN))
        {
          num_items++;
          shields += shield_rating(it);
          if (it->sort == shield)
            {
              shields += SHIELD_POWER*it->efficiency;
            }
        }
      it = items + it->link;
    }
  return shields / num_items;
}



double
module_damage (struct PLAYER *attacker, double shields, int range)
{
  int phase;
  const double ln2 = log(2.0);
  double total = 0.0;

  for (phase = 0 ; phase < 7 ; phase++)
    {
      double damage = damage_scored(attacker, range, phase);
      damage /= shields;
      if (damage > 1.0)
        damage = log(1.0 + damage)/ln2;
      total += damage;
    }
  return total;
}



/* look for combination of best relative damage, and doing _some_ damage */
int
find_best_range (struct PLAYER *attacker, struct PLAYER *defender)
{
  int range, best_range = 6;
  double score, max_score = 0.0;
  double ashields, dshields;

  ashields = average_shields(attacker);
  dshields = average_shields(defender);
  
  for (range = 0; range <= 6; range++)
    {
      score = module_damage(attacker, dshields, range) -
        module_damage(defender, ashields, range);
      if (score > max_score)
        {
          max_score = score;
          best_range = range;
        }
    }
  return (best_range);
}

int
any_good_range (struct PLAYER *attacker, struct PLAYER *defender)
{
  int range = find_best_range (attacker, defender);
  double ashields, dshields;

  ashields = average_shields(attacker);
  dshields = average_shields(defender);
  
  return (module_damage (attacker, dshields, range) >
          module_damage (defender, ashields, range));
}

/* chooses dice among targetted if any (but always chooses demanded item
   if any), otherwise dice from any */
int
inner_choose_target (struct ITEM *item)
{
  int num_items = 0, num_targets = 0;
  struct ITEM *it = item;
  int count;

  while (it != items)
    {
      num_items++;
      if (it->flags & ITEM_TARGETTED)
        {
          if (it->flags & ITEM_DEMANDED)
            return (it - items);
          num_targets++;
        }
      it = items + it->link;
    }
  it = item;
  if (num_targets)
    count = dice (num_targets) + 1;
  else
    count = dice (num_items) + 1;
  while (count)
    {
      if (it->flags & ITEM_TARGETTED || num_targets == 0)
        {
          count--;
          if (count == 0)
            return (it - items);
        }
      it = items + it->link;
    }
  printf ("Couldn't find a target after all\n");
  return 0;
}


/* chooses dice among targetted if any (but always chooses demanded item
   if any), otherwise dice from any, avoiding broken ones mostly */
int
choose_target (struct ITEM *item)
{
  int retry = 5, it;

  while (retry--)
    {
      it = inner_choose_target (item);
      if (!(items[it].flags & ITEM_BROKEN))
        return (it);
    }
  return (inner_choose_target (item));
}


int
shield_rating (struct ITEM *item)
{
  if (item->sort == artifact)
    return (200);
  else
    return (25 * item->efficiency);
}


int
find_longest_weapon (struct ITEM *item)
{
  int result = -1;

  do
    {
      if (!(item->flags & ITEM_BROKEN))
        switch (item->sort)
          {
          case ram:
            if (result < 0)
              result = 0;
            break;
          case gun:
            if (result < 1)
              result = 1;
            break;
          case disruptor:
            if (result < 2)
              result = 2;
            break;
          case laser:
            if (result < 3)
              result = 3;
            break;
          case missile:
            if (result < 4)
              result = 4;
            break;
          case drone:
            if (result < 5)
              result = 5;
            break;
          case fighter:
            if (result < 6)
              result = 6;
            break;
          case warp_drive:
          case impulse_drive:
          case sensor:
          case cloak:
          case life_support:
          case sick_bay:
          case shield:
          case pod:
          case artifact:
          case evil_artifact:
          default:
            break;
          }
      item = items + item->link;
    }
  while (item != items);
  return (result);
}

int
torp_damage (struct PLAYER *player, int range)
{
  int salvo;

  if (player->torps >=
      player->strategy.firing_rate * player->strategy.firing_rate)
    salvo = player->strategy.firing_rate;
  else
    salvo = isqrt (player->torps);
  return (2 * salvo);
}

int
fire_torps (struct PLAYER *player, int range)
{
  int salvo;

  if (player->torps >=
      player->strategy.firing_rate * player->strategy.firing_rate)
    salvo = player->strategy.firing_rate;
  else
    salvo = isqrt (player->torps);
  player->torps -= salvo * salvo;
  return (2 * salvo);
}

int
damage_scored (struct PLAYER *player, int range, int phase)
{
  int result = 0;
  struct ITEM *item = items + player->ship;

  do
    {
      if ((item->flags & ITEM_BROKEN) == 0)
        switch (item->sort)
          {
          case ram:
            if (range == 0)
              result += item->efficiency * 5;
            break;
          case gun:
            if (range <= 1 && phase >= 1)
              result += item->efficiency * 5;
            break;
          case disruptor:
            if (range <= 2 && phase >= 2)
              result += item->efficiency * 5;
            break;
          case laser:
            if (range <= 3 && phase >= 3)
              result += item->efficiency * 5;
            break;
          case missile:
            if (range <= 4 && phase >= 4)
              result += item->efficiency * 5;
            break;
          case drone:
            if (range <= 5 && phase >= 5)
              result += item->efficiency * 5;
            break;
          case fighter:
            if (range <= 6 && phase >= 6)
              result += item->efficiency * 5;
            break;
          case warp_drive:
          case impulse_drive:
          case sensor:
          case cloak:
          case life_support:
          case sick_bay:
          case shield:
          case pod:
          case artifact:
          case evil_artifact:
            break;
          }
      item = items + item->link;
    }
  while (item != items);
  result = blessing_mod (player, result, ram);
  return (result);
}


void
resolve_reserve_shields (FILE * fd, struct PLAYER *defender, int target)
{
  if (items[target].flags & ITEM_PROTECTED)
    {
      defender->shields += defender->reserve;
      fprintf (fd, "<BR>%s's reserve shields engaged\n",
               name_string (defender->name));
    }
}

int
ship_boom (struct PLAYER *ship)
{
  struct ITEM *item = items + ship->ship;
  int result = 0;

  while (item != items && item->sort < pod)
    {
      result += item->efficiency;
      item = items + item->link;
    }
  return (result);
}

int
resolve_kamikaze (FILE * fd, struct PLAYER *attacker, struct PLAYER *defender,
                  int range)
{
  int result;
  int attacker_impulse, defender_impulse;

  if (attacker->alliance == PLAYER_ALLIANCE ||
      attacker->star != homeworlds[attacker->alliance])
    return (0);                 /* not alien fighting for homeworld */
  if (damage_scored (attacker, range, 6) + torp_damage (attacker, range) >=
      damage_scored (defender, range, 6) + torp_damage (defender, range))
    return (0);                 /* doing OK so far */
  attacker_impulse = favour_option (attacker, favour_engines,
                                    factor (impulse_drive, attacker));
  defender_impulse = favour_option (defender,favour_engines,
                                    factor (impulse_drive, defender));
  if (range > 0 &&
      attacker_impulse > defender_impulse)
    return (0);                 /* getting closer for bigger bang */
  fprintf (fd, "<BR>%s self destructs in kamikaze attack!",
           attacker->name);
  result = ship_boom (attacker) * (7 - range) * 10;
  destroy_ship (attacker);
  return (result);
}

int
shield_strength (struct PLAYER *player, int basic)
{
  int result;

  result = favour_option (player, favour_shields, basic);
  result = blessing_mod (player, result, shield);
  return (result);
}


struct PLAYER *
resolve_strike (FILE * fd, struct PLAYER *attacker, struct PLAYER *defender,
                int range, int phase)
{
  int hits, salvo, shields;
  struct ITEM *it;

  salvo = fire_torps (attacker, range);
  hits =
    favour_option (attacker, favour_weapons,
                   damage_scored (attacker, range, phase));
  hits += resolve_kamikaze (fd, attacker, defender, range);
  hits += salvo;
  fprintf (fd,
           "<BR><STRONG>%s does %d damage (including %d with torpedoes)</STRONG>\n",
           name_string (attacker->name), hits, salvo);
  defender->shields -= hits;
  while (defender->shields <= 0)
    {
      fprintf (fd, "<BR>%s is hit, %s is lost\n",
               name_string (defender->name),
               item_string (items + attacker->strategy.target));
      attacker->favour[weaponry] +=
        items[attacker->strategy.target].efficiency;
      add_favour (attacker, weaponry,
                  items[attacker->strategy.target].efficiency);
      defender->health -= dice (20);
      if (defender->health < 0)
        defender->health = 0;
      defender->losses++;

      it = items + attacker->strategy.target;

      if (it->sort == artifact)
        attacker->damage += 4;
      else
        attacker->damage += it->efficiency;
      
      if ((it->sort == shield) && ((it->flags & ITEM_BROKEN) == 0))
        {
          int loss;

          loss = it->efficiency * defender->reserve_per_item;
          fprintf (fd,
                   "<BR>Reserve shields reduced by %d points per protected module\n",
                   loss);
          defender->reserve -= loss;
        }

      save_loot (attacker->strategy.target, defender - ships);
      if (it->sort == artifact)
        consolidate_artifacts ();
      if (defender->ship == 0)      /* dead */
        {
          fprintf (fd, "<BR><STRONG>%s's ship destroyed!</STRONG>\n",
                   name_string (defender->name));
          if (defender < aliens && !(defender->preferences & 8))
            parse_order ("y=2", defender - players);
          return attacker;
        }
      attacker->strategy.target = choose_target (items + defender->ship);
      defender->shields /= 2;
      fprintf (fd, "<BR>%s selects new target: %s\n",
               name_string (attacker->name),
               item_string (items + attacker->strategy.target));

      defender->shields +=
        shield_strength (defender,
                         shield_rating (items + attacker->strategy.target));
      resolve_reserve_shields (fd, defender, attacker->strategy.target);
      if (defender->strategy.dip_option == flee)
        {
          int m = mass(defender);
          int r = 4 + range;
          int fp = (1140 * r)/(m + 28); /* fp = 5 r for m = 200
                                           fp = 10 r for m = 86
                                           fp = 20 r for m = 29
                                           fp = 30 r for m = 10 */
          fp = favour_option (defender, favour_fleeing, fp);
          defender->fleeing += fp;
        }
    }

  shields = shield_strength (defender,
                             shield_rating (items +
                                            attacker->strategy.target));
  if (shields)
    fprintf (fd, "<BR>%s's shields at %d%% (%d points)\n",
             name_string (defender->name),
             (defender->shields * 100) / shields, defender->shields);
  else
    fprintf (fd, "<BR>%s's shields down\n", name_string (defender->name));

  if (attacker->ship == 0)      /* destroyed */
    {
      return defender;
    }
  if (defender->ship == 0)      /* destroyed */
    {
      return attacker;
    }
  return 0;
}

int
resolve_range (FILE * fd, struct PLAYER *attacker, struct PLAYER *defender,
               int range)
{
  if (attacker->strategy.dip_option == always_attack &&
      damage_scored (attacker, attacker->strategy.ideal_range, 6) == 0 &&
      damage_scored (attacker, 0, 6) > 0)
    {
      do
        attacker->strategy.ideal_range--;
      while (!damage_scored (attacker, attacker->strategy.ideal_range, 6));
      fprintf (fd, "<BR>%s changes to shorter range strategy\n",
               name_string (attacker->name));
    }
  if (attacker->movement - defender->movement > 30 + range * 4 &&
      range != attacker->strategy.ideal_range)
    {
      attacker->movement -= 30 + range * 4;
      if (attacker->strategy.ideal_range > range)
        {
          fprintf (fd, "<BR><EM>%s widens range</EM>\n",
                   name_string (attacker->name));
          range++;
        }
      else
        {
          fprintf (fd, "<BR><EM>%s narrows range</EM>\n",
                   name_string (attacker->name));
          range--;
        }
    }
  return (range);
}

int
resolve_retreats (FILE * fd, struct PLAYER *attacker,
                  struct PLAYER *defender, int range)
{
  int threshold, am, dm;
  if (attacker->strategy.dip_option == always_attack)
    {
      if (attacker->losses >= attacker->strategy.retreat)
        {
          fprintf (fd,
                   "<BR><EM>%s has too much damage so tries to break off</EM>\n",
                   name_string (attacker->name));
          attacker->strategy.dip_option = flee;
        }
      if (find_longest_weapon (items + attacker->ship) == -1 &&
          (attacker->alliance == PLAYER_ALLIANCE ||
           attacker->star != homeworlds[attacker->alliance]))
        {
          fprintf (fd,
                   "<BR><EM>%s has no weapons so tries to break off</EM>\n",
                   name_string (attacker->name));
          attacker->strategy.dip_option = flee;
        }
    }
  else
    fprintf (fd, "<BR><EM>%s attempts to flee</EM>\n",
             name_string (attacker->name));
  am = mass(attacker);
  dm = mass(defender);
  threshold = 500 + (400 * (am - dm))/(am + dm);

  if (attacker->fleeing >= threshold)
    {
      fprintf (fd, "<H3>%s succeeds in fleeing from the battle!</H3><BR>\n",
               name_string (attacker->name));
      return (TRUE);
    }
  else
    return (FALSE);
}


int
detection_range (struct PLAYER *attacker, struct PLAYER *defender)
{
  int result;
  int delta;
  
  /* on holiday */
  if (attacker->star > MAX_STAR)
    {
      printf ("detection_range called for attacker on holiday/invalid star: %d\n",
              attacker->star);
      return 6;
    }
  
  result = stars[attacker->star].terrain;

  delta =
    (favour_option (attacker, favour_sensors, factor (sensor, attacker))
     - favour_option (defender, favour_cloaks,
                      factor (cloak, defender))) / 10;

  if (delta > 0)
    {
      result += (isqrt(1 + 8 * delta) - 1)/2;
    }
  else if (delta < 0)
    {
      result -= (isqrt(1 - 8 * delta) - 1)/2;
    }

  if (result < 0)
    result = 0;
  if (result > 6)
    result = 6;
  return (result);
}

int
stalemate (struct PLAYER *p1, struct PLAYER *p2, int range)
{
  if (damage_scored (p1, range, 6))
    return (FALSE);
  if (damage_scored (p2, range, 6))
    return (FALSE);
  if (range != p1->strategy.ideal_range &&
      favour_option (p1, favour_engines, factor (impulse_drive, p1)) >
      favour_option (p2, favour_engines, factor (impulse_drive, p2)))
    return (FALSE);
  if (range != p2->strategy.ideal_range &&
      favour_option (p2, favour_engines, factor (impulse_drive, p2)) >
      favour_option (p1, favour_engines, factor (impulse_drive, p1)))
    return (FALSE);
  return (TRUE);
}

void
check_luck (struct PLAYER *player)
{
  struct ITEM *item = items + player->ship;

  while (item != items)
    {
      if (item->flags & ITEM_LUCKY)
        item->flags &= ~ITEM_TARGETTED;
      item = items + item->link;
    }
}

  


int
resolve_combat (FILE * fd, struct PLAYER *attacker, struct PLAYER *defender)
{
  int range, attacker_range, defender_range;
  int round = 1, risk;
  struct PLAYER *winner = 0;
  int cost;
  int loot_size;

  if (attacker->star > MAX_STAR)
    {
      printf ("resolve_combat called for attacker on holiday/invalid star: %d\n",
              attacker->star);
      return FALSE;
    }

  if (defender->star > MAX_STAR)
    {
      printf ("resolve_combat called for defender on holiday/invalid star: %d\n",
              attacker->star);
      return FALSE;
    }


  attacker->movement = defender->movement = 0;
  attacker->damage = defender->damage = 0;
  check_luck (attacker);
  check_luck (defender);

  /* check for hiding/hunting */
  if (attacker->hide_hunt > 0 && defender->hide_hunt > 0)
    {
      fprintf (fd, "<P>Combat attempted but both ships hiding\n");
      return (TRUE);
    }
  if (attacker->hide_hunt > 0)
    {
      fprintf (fd, "<P>%s hides in %s\n",
               name_string (attacker->name),
               attacker->hide_hunt > 1 ?
               loc_string (attacker->hide_hunt) : "Space");
      risk = defender->standby == 15 ?
        risk_level (defender, abs (defender->hide_hunt)) : 0;
      if (risk < risk_level (defender, abs (attacker->hide_hunt)))
        {
          fprintf (fd, " and avoids combat\n");
          return (TRUE);
        }
      else
        fprintf (fd, " but %s hunts there\n", name_string (defender->name));
    }
  if (defender->hide_hunt > 0)
    {
      fprintf (fd, "<P>%s hides in %s\n",
               name_string (defender->name),
               defender->hide_hunt > 1 ?
               loc_string (defender->hide_hunt) : "Space");
      risk = attacker->standby == 15 ?
        risk_level (attacker, abs (attacker->hide_hunt)) : 0;
      if (risk < risk_level (attacker, abs (defender->hide_hunt)))
        {
          fprintf (fd, " and avoids combat\n");
          return (TRUE);
        }
      else
        fprintf (fd, " but %s hunts there\n", name_string (attacker->name));
    }
  if (attacker->hide_hunt < 0 && defender->hide_hunt < 0)
    {
      fprintf (fd, "<P>Combat attempted and both ships hunting\n");
    }
  /* check for making a new enemy, neutrals bear grudges,
     friendlies and chaotics don't */
  if (attacker->alliance == PLAYER_ALLIANCE && defender->alliance != PLAYER_ALLIANCE)   /* player attacker */
    {
      if (races[defender->alliance].hostility != friendly &&
          races[defender->alliance].hostility != chaotic)
        attacker->enemies |= 1 << defender->alliance;
    }
  else
    if (attacker->alliance != PLAYER_ALLIANCE &&
        attacker->star == homeworlds[attacker->alliance])
      {
        fprintf (fd, "<P>%s defending homeworld heroically\n", attacker->name);
        attacker->strategy.dip_option = always_attack;
        attacker->strategy.ideal_range = 0;
        /* smarter aliens */
        if (dice (3) == 0)
          attacker->strategy.cbt_option = dice (6);
        else
          attacker->strategy.cbt_option = favour_engines;
        attacker->strategy.cbt_option = favour_engines;
        attacker->strategy.retreat = BIG_NUMBER;
      }
  if (defender->alliance == PLAYER_ALLIANCE && attacker->alliance != PLAYER_ALLIANCE)   /* player defender */
    {
      if (races[attacker->alliance].hostility != friendly &&
          races[attacker->alliance].hostility != chaotic)
        defender->enemies |= 1 << attacker->alliance;
    }
  else
    if (defender->alliance != PLAYER_ALLIANCE &&
        defender->star == homeworlds[defender->alliance])
      {
        fprintf (fd, "<P>%s defending homeworld heroically\n", defender->name);
        defender->strategy.dip_option = always_attack;
        defender->strategy.ideal_range = 0;
        /* smarter aliens */
        if (dice (3) == 0)
          defender->strategy.cbt_option = dice (6);
        else
          defender->strategy.cbt_option = favour_engines;
        defender->strategy.cbt_option = favour_engines;
        defender->strategy.retreat = BIG_NUMBER;
      }

  if (is_player (defender) && is_wraith (attacker))
    {
      if (resolve_ring_interaction (fd, attacker, defender))
        {
          return (TRUE);
        }
    }
  if (is_player (attacker) && is_wraith (defender))
    {
      if (resolve_ring_interaction (fd, defender, attacker))
        {
          return (TRUE);
        }
    }

  if (attacker->magic_flags & FLAG_AVOID_COMBAT)
    {
      fprintf (fd, "<BR>%s uses micro-jump to avoid combat\n",
               name_string (attacker->name));
      if (defender->magic_flags & FLAG_FORCE_COMBAT)
        fprintf (fd, "<BR>but %s anticipates and engages\n",
                 name_string (defender->name));
      else
        return (TRUE);
    }

  if (defender->magic_flags & FLAG_AVOID_COMBAT)
    {
      fprintf (fd, "<BR>%s uses micro-jump to avoid combat\n",
               name_string (defender->name));
      if (attacker->magic_flags & FLAG_FORCE_COMBAT)
        fprintf (fd, "<BR>but %s anticipates and engages\n",
                 name_string (attacker->name));
      else
        return (TRUE);
    }

  if (defender->strategy.dip_option == flee)
    defender->strategy.ideal_range = 6;
  if (defender->strategy.dip_option == no_attack)
    defender->strategy.dip_option = always_attack;

  attacker_range = min (attacker->strategy.ideal_range,
                        detection_range (attacker, defender));
  defender_range = min (defender->strategy.ideal_range,
                        detection_range (defender, attacker));
  range = max (attacker_range, defender_range);

  fprintf (fd, "<H2>%s attacks \n", name_string (attacker->name));
  fprintf (fd, "%s in %s terrain, opening fire at %s range</H2>\n",
           name_string (defender->name),
           terrain_names[stars[attacker->star].terrain], range_names[range]);
  fprintf (fd, "<P><EM>%s's strategy favours %s,\n",
           name_string (attacker->name),
           favour_names[attacker->strategy.cbt_option]);
  fprintf (fd, "%s's strategy favours %s</EM>\n",
           name_string (defender->name),
           favour_names[defender->strategy.cbt_option]);

  attacker->strategy.target = choose_target (items + defender->ship);
  defender->strategy.target = choose_target (items + attacker->ship);

  attacker->shields = shield_strength (attacker,
                                       shield_rating (items +
                                                      defender->strategy.
                                                      target));
  defender->shields =
    shield_strength (defender,
                     shield_rating (items + attacker->strategy.target));
  attacker->reserve =
    shield_strength (attacker,
                     (SHIELD_POWER * total_working_item (shield,
                                                         items + attacker->ship))) /
    protected_items (attacker->ship);
  attacker->reserve_per_item =
    shield_strength (attacker,
                     SHIELD_POWER) / protected_items (attacker->ship);
  defender->reserve =
    shield_strength (defender,
                     (SHIELD_POWER * total_working_item (shield,
                                                         items + defender->ship))) /
    protected_items (defender->ship);
  defender->reserve_per_item =
    shield_strength (defender,SHIELD_POWER) / protected_items (defender->ship);
  fprintf (fd, "<BR>%s targets: %s\n", name_string (attacker->name),
           item_string (items + attacker->strategy.target));
  resolve_reserve_shields (fd, defender, attacker->strategy.target);
  fprintf (fd, "<BR>%s targets: %s\n",
           name_string (defender->name),
           item_string (items + defender->strategy.target));
  resolve_reserve_shields (fd, attacker, defender->strategy.target);

  loot_size = ship_size (items + attacker->ship)
    + ship_size (items + defender->ship);

  loot_array = malloc (loot_size * sizeof(int));
  loot_max = 0;
  
  while (!winner)
    {
      int i;
      fprintf (fd, "<H3>Round %d, range is %s</H3>\n",
               round, range_names[range]);

      for (i = range ; i < 7 ; i++)
        {
          fprintf (fd, "<H4>Phase %d</H4>", i + 1);

          winner = resolve_strike (fd, attacker, defender, range, i);
          if (winner)
            break;
          if (resolve_retreats (fd, defender, attacker, range))
            {
              winner = attacker;
              break;
            }
          fprintf (fd, "<P>");     /* tidy output */
          winner = resolve_strike (fd, defender, attacker, range, i);
          if (winner)
            break;
          if (resolve_retreats (fd, attacker, defender, range))
            {
              winner = defender;
              break;
            }
        }
      fprintf (fd, "<BR>");     /* tidy output */
      if (winner)
        break;
      if (attacker->strategy.dip_option == flee)
        {
          attacker->strategy.ideal_range = 6;
          attacker->fleeing += favour_option (attacker, favour_fleeing,
                                              20 + 5 * range);
        }
      if (defender->strategy.dip_option == flee)
        {
          defender->strategy.ideal_range = 6;
          defender->fleeing += favour_option (defender, favour_fleeing,
                                              20 + 5 * range);
        }

      if (resolve_retreats (fd, attacker, defender, range))
        winner = defender;
      else if (resolve_retreats (fd, defender, attacker, range))
        winner = attacker;

      if (winner)
        break;

      /* range change */
      attacker->movement +=
        favour_option (attacker, favour_engines,
                       factor (impulse_drive, attacker));
      defender->movement +=
        favour_option (defender, favour_engines,
                       factor (impulse_drive, defender));
      range = resolve_range (fd, attacker, defender, range);
      range = resolve_range (fd, defender, attacker, range);

      round++;

      if (stalemate (attacker, defender, range) || round > 500)
        {
          fprintf (fd,
                   "<H2>Neither ship able to make progress after %d rounds, combat ends\n",
                   --round);
          if (attacker->fleeing > defender->fleeing)
            {
              fprintf (fd, " with %s fleeing</H2>\n",
                       name_string (attacker->name));
              winner = defender;
            }
          else if (defender->fleeing > attacker->fleeing)
            {
              fprintf (fd, " with %s fleeing</H2>\n",
                       name_string (defender->name));
              winner = attacker;
            }
          else
            {
              fprintf (fd, " with any loot lost to both</H2>\n");
              destroy_loot ();
              free (loot_array);
              return (TRUE);
            }
        }
    }
  fprintf (fd, "<H2>%s wins</H2>\n",
           name_string (winner->name));
  cost = split_loot (fd, winner, (winner == attacker) ? defender : attacker);
  if (is_player (attacker) && is_player (defender))
    {
      fprintf (times,
               "<hr>(Debug) Battle between players with total damage value: $%d\n",
               cost);
      printf ("Battle: %s vs %s\n", attacker->name, defender->name);
    }
  free (loot_array);
  return (TRUE);
}

void
ring_clash (FILE * fd, int skill,
            struct PLAYER *attacker, struct PLAYER *defender)
{
  printf ("RING CLASH!\n");
  attacker->rings_held &= ~(0x1 << skill);
  init_ring (skill);
  defender->rings_held &= ~(0x10 << skill);
  init_ring (skill + 4);
  fprintf (fd, "<P>%s rings of both ships destroyed!\n", skill_names[skill]);
}


int
steal_skills (FILE * fd, int skill,
              struct PLAYER *attacker, struct PLAYER *defender)
{
  int extra_skills, bit;

  extra_skills = defender->skills[skill] & ~attacker->skills[skill];
  attacker->skills[skill] |= extra_skills;
  defender->skills[skill] &= ~extra_skills;
  fprintf (fd, "<BR>%s uses Evil %s ring to steal skills from ",
           name_string (attacker->name), skill_names[skill]);
  fprintf (fd, "%s: \n", name_string (defender->name));
  if (extra_skills == 0)
    fprintf (fd, "None");
  else
    for (bit = 0; bit < 32; bit++)
      if (extra_skills & (1 << bit))
        show_skill (fd, skill, bit);
  return (extra_skills);
}

int
resolve_ring_interaction (FILE * fd,
                          struct PLAYER *attacker, struct PLAYER *defender)
{
  int skill;
  int result = 0;

  printf ("Resolving ring combat for %s v %s\n",
          attacker->name, defender->name);
  for (skill = 0; skill < 4; skill++)
    {
      if (attacker->rings_held & (0x1 << skill))
        {
          if (defender->rings_held & (0x10 << skill))
            {
              result |= 1;
              ring_clash (fd, skill, attacker, defender);
            }
          else
            result |= steal_skills (fd, skill, attacker, defender);
        }
    }
  return (result);
}


int
power (struct PLAYER *player)
{
  int total = 0;
  struct ITEM *ship = items + player->ship;

  if (player == dybuk)
    total += BIG_NUMBER; /* Lots of evil counts for a lot */
  
  do
    {
      if (!(ship->flags & (ITEM_BROKEN | ITEM_DEMO)))
        total += ship->efficiency;      /* measure of power ? */
      if (ship->sort == evil_artifact  )
        total += 15;    /* some evil isn't insignifigant either */
      ship = items + ship->link;
    }
  while (ship != items);
  if (player->powermod == 1)
    total += total / 4;
  if (player->powermod == -1)
    total -= total / 4;
  return (total);
}

int
escorted (struct PLAYER *player)
{
  struct PLAYER *escort = players + player->companion;

  if (player->companion == 0)
    return (FALSE);
  if (escort->companion != player - players)
    return (FALSE);
  if (player->star != escort->star)
    return (FALSE);
  if (power (escort) < power (player))
    return (FALSE);
  if (power (escort) == power (player) && escort > player)
    return (FALSE);
  return (TRUE);
}


int
favour_option (struct PLAYER *player, combat_option option, int basic)
{
  if (player->strategy.cbt_option == option)
    return ((basic * 3) / 2);
  else
    return (basic);
}


