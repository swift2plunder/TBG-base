#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>             /* for sleep */
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <math.h> /* because I don't hate floating point */

#include "defs.h"
#include "tbg.h"
#include "bytes.h"
#include "rand.h"
#include "util.h"
#include "orders.h"
#include "globals.h"
#include "politics.h"
#include "data.h"
#include "items.h"
#include "locations.h"
#include "dybuk.h"
#include "combat.h"
#include "criminals.h"
#include "adventures.h"
#include "ranking.h"
#include "aliens.h"
#include "cargo.h"
#include "skill.h"
#include "tbg-big.h"
#include "religion.h"
#include "crew.h"
#include "movement.h"

/*#define SYSTEM(command) { int rc = system(command); }*/
#define SYSTEM(command) { system(command); }

int
mothballed (int p)
{
  return (turn - players[p].last_orders) > 50;
}

void
print_rules_link (FILE *fd, const char *link, const char *text)
{
  fprintf(fd,"<A HREF=\"http://%s/Rules.html#%s\">", server, link);
  fprintf(fd,"%s</A>", text);
}

/* maps full range of module types down to generic 0-9 */
int
module_type (int sort)
{
  switch (sort)
    {
    case warp_drive:
    case impulse_drive:
    case sensor:
    case cloak:
    case life_support:
    case sick_bay:
    case shield:
      return (sort);
    case ram:
    case gun:
    case disruptor:
    case laser:
    case missile:
    case drone:
    case fighter:
      return (7);
    case pod:
      return (8);
    case artifact:
      return (9);
    case evil_artifact:
      return (10);
    default:
      printf("Error in module_type, nonexistant module: %d\n", sort);
      return (-1);
    }
}

const char *
ring_string (int ring)
{
  static char buffer[256];
  int index = 0;

  while ((ring & (1 << index)) == 0)
    index++;
  sprintf (buffer, "%s %s Ring",
           index < 4 ? "Evil" : "Good", skill_names[index & 3]);
  return (buffer);
}


void
show_good_artifact (FILE * fd, struct ITEM *item)
{
  int i;

  fprintf (fd, "<TD>%s</TD>", item_string (item));
  for (i = 0; i < 8; i++)
    if (item->magic & (0x10000 << i))
      fprintf (fd, "<TD>%s</TD>", short_item_names[i]);
  fprintf (fd, "<TD>");
  for (i = 0; i < 8; i++)
    if (item->magic & (0x100 << i))
      fprintf (fd, "%s", short_item_names[i]);
  if ((item->magic & 0xff00) == 0)
    fprintf (fd, "None");
  fprintf (fd, "</TD><TD>");
  for (i = 0; i < 8; i++)
    if (item->magic & (1 << i))
      fprintf (fd, "%d", i);
  fprintf (fd, "</TD>");
  fprintf (fd, "\n");
}

void
show_bad_artifact (FILE * fd, struct ITEM *item)
{
  fprintf (fd, "<TD>%s</TD>", item_string (item));
  fprintf (fd, "<TD>%d</TD><TD>%d</TD><TD>%d</TD>",
           (item->magic >> 20) & 0x03ff,
           (item->magic >> 10) & 0x03ff,
           item->magic & 0x03ff);
}


void
show_item (FILE * fd, short it)
{
  struct ITEM *item = items + it;

  if (it == 0)
    return;
  fprintf (fd, "<TR ALIGN=CENTER>");
  switch (item->sort)
    {
    default:
      fprintf (fd, "<TD>%s</TD><TD>%s</TD><TD>%2d%%</TD><TD>%d</TD>",
               item_string (item),
               tech_level_names[item->efficiency],
               item->reliability, 3 - item->collection);
      break;
    case pod:
      fprintf (fd, "<TD>%s</TD><TD>%d</TD><TD>%s</TD><TD>%d</TD>",
               item_string (item),
               item->efficiency,
               item->reliability < BASE_UNIT ?
               goods[item->reliability].name :
               units[item->reliability - BASE_UNIT].name, item->collection);
      break;
    case artifact:
      show_good_artifact (fd, item);
      break;
    case evil_artifact:
      show_bad_artifact (fd, item);
      break;
    }
  fprintf (fd, "</TR>");
  fprintf (fd, "\n");
}

int
total_collection (struct ITEM *ship)
{
  struct ITEM *item = ship;
  int collection = 0;

  while (item != items)
    {
      if (item->sort < pod && !(item->flags & ITEM_BROKEN))
        collection += (3 - item->collection);
      item = items + item->link;
    }
  return (collection);
}

int
blessing_mod (struct PLAYER *player, int result, item_sort sort)
{
  /* and up or down for artifacts */
  if (sort >= ram && sort <= fighter)
    sort = ram;                 /* treat all weapons the same */
  if (player->artifacts & (0x10000 << sort))
    {
      result = (result * 3) / 2;  /* bonus 50% */
    }
  else if (player->chosen & (1 << repairers[sort]))
    {
      result = (result * 5) /4;  /* bonus 25% */
    }
    
  if (player->artifacts & (0x100 << sort))
    {
      result /= 2;                /* penalty 50% */
    }
  else if (player->heretic & (1 << repairers[sort]))
    {
      result = (result * 3) /4;  /* penalty 25% */
    }
  return result;
}


double
effective_range (double r, double f)
{
  double c = 2000000000;
  if (f > 0)
    c = exp((0.4 * r)/log(f))/200.0;
  return (c > 2000000000) ? 2000000000 :  c;
}

     

void
show_ship (FILE * fd, struct PLAYER *ship)
{
  struct ITEM *item = items + ship->ship;
  int collection = 0;
  int cargo = 0, total = 0;
  char buffer[512];
  char flag[128];
  FILE *user;                   /* player web page, if any */
  int link = FALSE;

  if (ship->account_number >= 200)
    {
      sprintf (buffer, "%s/players/%d.html",
               webroot, ship->account_number);
      user = fopen (buffer, "r");
      if (user)
        {
          fclose (user);
          link = TRUE;
        }
    }

  if (ship->banner[0] != '<')   /* not a picture, put it in brackets */
    sprintf (flag, "(%s)", ship->banner);
  else
    strcpy (flag, ship->banner);
  if (link)
    sprintf (buffer, "<a href=\"http://%s/players/%d.html\">%s</a> %s",
             server, ship->account_number, name_string (ship->name), flag);
  else
    sprintf (buffer, "%s %s", name_string (ship->name), flag);
  fprintf (fd, "<A NAME=\"%s\"></A>\n", ship->name);
  fprintf (fd, "<TABLE BORDER=1>\n");
  fprintf (fd, "<TR><TH COLSPAN=4 ALIGN=CENTER>%s</TH></TR>\n", buffer);
  if (ship->ship)
    {
      if (item->sort < pod)
        {
          fprintf (fd,
                   "<TR><TH>Component</TH><TH>Tech</TH><TH>Reliability</TH><TH>E Cost</TH></TR>\n");
          do
            {
              show_item (fd, item - items);
              total++;
              item = items + item->link;
            }
          while (item != items && item->sort < pod);
        }
      if (item->sort == pod)
        {
          fprintf (fd,
                   "<TR><TH>Component</TH><TH>Capacity</TH><TH>Cargo</TH><TH>Amount</TH></TR>\n");
          do
            {
              show_item (fd, item - items);
              total++;
              total += item->collection;
              cargo += item->efficiency;
              item = items + item->link;
            }
          while (item != items && item->sort < artifact);
        }
      if (item->sort == artifact)
        {
          fprintf (fd,
                   "<TR><TH>Artifact</TH><TH>Bless</TH><TH>Curse</TH><TH>Keys</TH></TR>\n");
          do
            {
              show_item (fd, item - items);
              total++;
              item = items + item->link;
            }
          while (item != items && item->sort < evil_artifact);
        }
      if (item->sort == evil_artifact)
        {
          fprintf (fd,
                   "<TR><TH>Evil Zapper</TH><TH>Biopedos</TH><TH>Logipedos</TH><TH>Bangpedos</TH></TR>\n");
          do
            {
              show_item (fd, item - items);
              total++;
              item = items + item->link;
            }
          while (item != items);
        }
    }
  fprintf (fd, "</TABLE>\n");
  if (ship->prisoner)
    fprintf (fd, "<P>Holding %s prisoner\n",
             criminal_string (ship->prisoner));
  if (ship->medicine)
    fprintf (fd, "<P>Holding %s Medicine Value $%d\n",
             races[ship->medicine / 1000].name, 50*(ship->medicine % 1000));
  collection = total_collection (items + ship->ship);
  fprintf (fd, "<P><STRONG>Mass = %d, Energy Cost = %d, ",
           total, collection);
  fprintf (fd, "Torpedo Stock = %d, ", ship->torps);
  fprintf (fd, "Cargo capacity: %d ", cargo);
  fprintf (fd, "</STRONG><P>\n");
}


/* returns byte, race in high 5 bits, rank in low 3 */
int
new_criminal (int loc)
{
  int result;

  if (dice (3))
    return (NO_CRIMINAL);
  switch (locations[loc].sort)  /* race depends on location sometimes */
    {
    case homeworld:
      result = locations[loc].parameter << 3;
      break;
    case colony:
      result = RACE_NUMBER (locations[loc].parameter) << 3;
      break;
    default:
      result = dice (32) << 3;
    }
  return (result | dice (8));
}

void
init_ring (int ring)
{
  int loc;

  do
    loc = dice (MAX_LOCATION);
  while (locations[loc].ring ||
         locations[loc].star < 0 ||
         !(location_types[locations[loc].sort].flags & LOC_ADVENTURE));
  locations[loc].ring = 1 << ring;
  printf ("New Ring %d in %s at %s\n", ring, loc_string (loc),
          star_names[locations[loc].star]);
}

void
init_rings ()
{
  int r, loc;

  for (loc = 0; loc < MAX_LOCATION; loc++)
    locations[loc].ring = 0;
  for (r = 0; r < MAX_RING; r++)
    init_ring (r);
}

void
relocate (int loc)
{
  int oldstar = locations[loc].star;
  int sort = locations[loc].sort;
  int parameter = locations[loc].parameter;
  int star;
  int i;

  /* A HW's attached schools always stay attached */
  if ((sort == school) &&
      (locations[loc - parameter -1].sort == homeworld))
    {
      star = locations[loc - parameter -1].star;
      locations[loc].star = star;
      if (stars[star].loc_mask & location_types[sort].exclusion_bit)
        {
          /* should probably do something here */
        }
      stars[oldstar].loc_mask &= ~location_types[sort].exclusion_bit;
      stars[star].loc_mask |= location_types[sort].exclusion_bit;
      return;
    }

  do
    {
      star = dice (location_types[sort].max_star);
    }
  while (stars[star].loc_mask & location_types[sort].exclusion_bit
         && star != oldstar
         && (sort == academy
             && ! (stars[star].loc_mask
                   & location_types[homeworld].exclusion_bit)));
  
  stars[oldstar].loc_mask &= ~location_types[sort].exclusion_bit;
  stars[star].loc_mask |= location_types[sort].exclusion_bit;
  locations[loc].star = star;

  if (loc_type (loc, LOC_RISK))
    locations[loc].risk = rand_exp (75);
  else
    locations[loc].risk = 0;

  switch (sort)
    {
    case homeworld:
      /* each HW has a full set of schools, when a HW moves in, take
       * it's schools along, relocate any that were already there */
      
      if (stars[star].loc_mask |= location_types[school].exclusion_bit)
        {
          for (i = 0 ; i < MAX_LOCATION ; i++)
            {
              if (locations[i].star == star &&
                  locations[i].sort == school)
                {
                  relocate(i);
                }
            }
        }
      /* A HW's schools occupy the next 4 location slots */
      for (i = 1 ; i <= 4 ; i++)
        locations[loc + i].star = star;
      stars[star].loc_mask |= location_types[sort].exclusion_bit;
      /* update alien homeworld data */
      for (i = 0; i < 32; i++)
        {
          if (homeworlds[i] == oldstar)
            {
              homeworlds[i] = star;
            }
        }
      /* Academies are always at homeworlds, if there was one at the
       * old star, move it to a new random HW (to change things up) */
      for (i = 0 ; i < MAX_LOCATION ; i++)
        {
          if (locations[i].star == oldstar
              && locations[i].sort == academy)
            relocate(i);
        }
      break;
    case stargate:
      /* other stargate is either next or previous location */
      {
        int o;
        for (o = loc -1 ; o <= loc +1 ; loc++)
          {
            if (o != loc
                && locations[o].sort == stargate
                && locations[o].parameter == oldstar
                && locations[loc].parameter == locations[o].star)
              {
                locations[o].parameter = star;
              }
          }
      }
      break;
    default:
      break;
    }
}


void
supernova (int star)
{
  int loc;
  int p;
  int x,y;
  int failed = 1;

  if (star < 0 || star > MAX_STAR)
    return;
  
  fprintf (times, "<HR>Supernova at %s!!!", star_names[star]);

  reset_bit (public_stars, star);
  for (p = 0 ; p < MAX_PLAYER ; p++)
    {
      if (players[p].probe == star)
        players[p].probe = NOWHERE;
      reset_bit (players[p].stars, star);
    }

  for (loc = 0 ; loc < MAX_LOCATION ; loc++)
    {
      if (locations[loc].star == star)
        relocate(loc);
    }
  x = stars[star].x;
  y = stars[star].y;
  board[x][y] = 0xff;
  stars[star].terrain = 6;
  while (failed)
    {
      int i;
      failed = FALSE;
      x = dice (MAXX - NAME_SIZE);
      y = dice (MAXY);
      for (i = max (0, x - NAME_SIZE); i < min (MAXX, x + NAME_SIZE); i++)
        {
          if (board[i][y] != 0xff)
            failed = TRUE;
        }
    }
  stars[star].x = x;
  stars[star].y = y;
  board[x][y] = star;
}

void
generate_board ()
{
  int x, y, star, next, skill, rank, race;
  int i, failed, loc;
  location_sort sort;

  memset (board, 0xff, sizeof (board));
  memset (locations, 0, sizeof (locations));
  for (star = 0; star < MAX_STAR; star++)
    {
      stars[star].loc_mask = 0;
      do
        {
          failed = FALSE;
          x = dice (MAXX - NAME_SIZE);
          y = dice (MAXY);
          for (i = max (0, x - NAME_SIZE); i < min (MAXX, x + NAME_SIZE); i++)
            {
              if (board[i][y] != 0xff)
                failed++;
            }
          stars[star].x = x;
          stars[star].y = y;
        }
      while (failed);
      board[x][y] = star;
    }
  next = 0;
  for (sort = 1; sort < last_location_type; sort++)
    for (loc = 0; loc < location_types[sort].max_number; loc++)
      {
        do
          {
            star = dice (location_types[sort].max_star);
          }
        while (stars[star].loc_mask & location_types[sort].exclusion_bit);
        stars[star].loc_mask |= location_types[sort].exclusion_bit;
        locations[next].star = star;
        locations[next].sort = sort;
        locations[next].parameter = loc;
        if (loc_type (next, LOC_RISK))
          locations[next].risk = dice (100);
        else
          locations[next].risk = 0;
        switch (sort)
          {
          case school:
            locations[next].parameter &= 3;
            break;
          case factory:
          case colony:
            if (GOOD_NUMBER (loc) == 0)
              next--;
            /* delete loc if no good */
            break;
          default:
            break;
          }
        next++;
        switch (sort)
          {
          case homeworld:
            homeworlds[loc] = star;
            for (skill = 0; skill < 4; skill++)
              {
                locations[next].star = star;
                locations[next].sort = school;
                locations[next].parameter = skill;
                stars[star].loc_mask |= location_types[school].exclusion_bit;
                next++;
              }
            if (loc >= 28)      /* kidnap academy */
              {
                locations[loc - 28].star = star;
              }
            break;
          case stargate:
            locations[next].star = loc;
            locations[next].sort = stargate;
            locations[next].parameter = star;
            next++;
            break;
          default:
            break;
          }
      }
  for (race = 0; race < 32; race++)
    {
      for (rank = 1; rank < 8; rank++)
        {
          do
            {
              loc = dice (next);
            }
          while (locations[loc].criminal ||
                 (!loc_type (loc, LOC_CRIMINAL)) ||
                 (locations[loc].sort == homeworld &&
                  locations[loc].parameter != race) ||
                 (locations[loc].sort == colony &&
                  RACE_NUMBER (locations[loc].parameter) != race));
          locations[loc].criminal = (race << 3) | rank;
        }
      for (rank = 0; rank < 4; rank++)  /* skill */
        {
          do
            loc = dice (next);
          while (!loc_type (loc, LOC_ROGUE));
          locations[loc].rogues = (race << 3) | (2 * rank) | 1;
        }
    }
  printf ("There are %d locations\n", next);
}

int
local_strength (unit_sort sort, location_sort terrain, int troops)
{
  int result = troops * unit_types[sort].strength;

  if (terrain == unit_types[sort].good_terrain)
    result *= 2;
  if (terrain == unit_types[sort].bad_terrain)
    result /= 2;
  return (result);
}

void
show_military (FILE * fd, struct PLAYER *player)
{
  int troops[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  int strengths[4] = { 0, 0, 0, 0 };
  /* ruins, badland, ocean, factory */
  struct ITEM *item = items + player->ship;
  unit_sort sort;
  int found_some = FALSE;

  while (item != items)
    {
      if (item->sort == pod && item->reliability >= BASE_UNIT)
        {
          sort = units[item->reliability - BASE_UNIT].sort;
          troops[sort]++;
          found_some = TRUE;
        }
      item = items + item->link;
    }
  if (!found_some)
    return;
  fprintf (fd, "<STRONG>Mercenary Strengths: \n");
  for (sort = regular_infantry; sort <= assault_mechs; sort++)
    {
      strengths[0] += local_strength (sort, ruins, troops[sort]);
      strengths[1] += local_strength (sort, badland, troops[sort]);
      strengths[2] += local_strength (sort, ocean, troops[sort]);
      strengths[3] += local_strength (sort, factory, troops[sort]);
    }
  fprintf (fd, "Ruins %d, Badlands %d, Ocean %d, Factory %d<BR>\n",
           strengths[0], strengths[1], strengths[2], strengths[3]);
  fprintf (fd, "<P></STRONG>\n");
}

void
show_factors (FILE * fd, struct PLAYER *player)
{
  fprintf (fd, "<STRONG>\n");
  fprintf (fd, "Warp %d%%, Impulse %d%%, Sensor %d%%, Cloak %d%%, ",
           factor (warp_drive, player), factor (impulse_drive, player),
           factor (sensor, player), factor (cloak, player));
  fprintf (fd, "Life Support %d%%, Sickbay %d%%, Shield %d%%, ",
           factor (life_support, player), factor (sick_bay, player),
           factor (shield, player));
  fprintf (fd, "Weapon %d%%\n", factor (ram, player));
  fprintf (fd, "<P></STRONG>\n");
  show_military (fd, player);
}


const char *
pair_string (struct PLAYER *player)
{
  static char buffer[256];
  char command[256];
  char buf1[256];
  char buf2[256];
  
  struct PLAYER *freighter = players + player->companion;
  int unique_url;

  if (player->companion != 0 &&
      freighter->companion == player - players &&
      player->star == freighter->star)
    {
      unique_url = rand32();

      snprintf (buf1, 256, "%s/results/%d/ship%d_%d.html",
                webroot, game, player - ships, turn);
      snprintf (buf2, 256, "%s/Ship_%s%d.htm",
                webroot, uint32_name (unique_url), turn);
      force_symlink (buf1,buf2);

      sprintf (buffer, "<A HREF=\"http://%s/Ship_%s%d.htm\">%s</A>",
               server,
               uint32_name (unique_url), turn, name_string (player->name));
      strcat (buffer, " (guarding ");

      unique_url = rand32();

      snprintf (buf1, 256, "%s/results/%d/ship%d_%d.html",
                webroot, game, freighter - ships, turn);
      snprintf (buf2, 256, "%s/Ship_%s%d.htm",
                webroot, uint32_name (unique_url), turn);
      force_symlink (buf1,buf2);

      sprintf (command, "<A HREF=\"http://%s/Ship_%s%d.htm\">%s</A>)",
               server,
               uint32_name (unique_url), turn, name_string (freighter->name));

      strcat (buffer, command);
    }
  else
    {
      unique_url = rand32();

      snprintf (buf1, 256, "%s/results/%d/ship%d_%d.html",
                webroot, game, player - ships, turn);
      snprintf (buf2, 256, "%s/Ship_%s%d.htm",
                webroot, uint32_name (unique_url), turn);
      force_symlink (buf1,buf2);

      sprintf (buffer, "<A HREF=\"http://%s/Ship_%s%d.htm\">%s</A>",
               server,
               uint32_name (unique_url), turn, name_string (player->name));
    }
  return (buffer);
}

void
show_other_ships (FILE * fd, struct PLAYER *player, int star)
{
  struct PLAYER *base;

  fprintf (fd, "<H2>Other ships here:</H2>\n");
  base = sort_ships (star);
  while (base && base->next)
    {
      fprintf (fd, "%s meets \n", pair_string (base));

      fprintf (fd, "%s<BR>\n", pair_string (base->next));
      base = base->next->next;
    }
  if (base)
    {
      fprintf (fd, "%s leftover<BR>\n", pair_string (base));
    }
  fprintf (fd, "<H3>Details</H3>\n");
  base = sort_ships (star);
  while (base)
    {
      if ((player->preferences & 256)
          || base == player || base == pairing (player))
        {
          show_ship (fd, base);
          show_factors (fd, base);
          if (base->companion != 0 &&
              players[base->companion].companion == base - players &&
              base->star == players[base->companion].star)
            {
              show_ship (fd, players + base->companion);
              show_factors (fd, players + base->companion);
            }
        }
      base = base->next;
    }
  for (base = shops; base < shops + MAX_SHOP; base++)
    if (base->star == star)
      show_ship (fd, base);
}

void
show_shop_options (FILE * fd, int shop)
{
  int link = shops[shop].ship;

  while (link)
    {
      fprintf (fd, "<OPTION VALUE=-%d>", link);
      fprintf (fd, "Buy %s %s (%d%%) for $%d\n",
               tech_level_names[items[link].efficiency],
               item_string (items + link),
               items[link].reliability, items[link].price);
      link = items[link].link;
    }
}


void
show_location_option (FILE * fd, struct PLAYER *player, int site)
{
  location_sort sort = locations[site].sort;
  byte parameter = locations[site].parameter;
  int level;
  skill_sort skill;

  switch (sort)
    {
    case arsenal:
      for (level = 0; level < 6; level++)
        {
          fprintf (fd, "<OPTION VALUE=%d%c>", site, 'A' + (1 << level));
          fprintf (fd, "Buy %d Photon Torpedoes for $%d\n",
                   1 << level, 10 * (1 << level));
        }
      break;
    case colony:
      if ((!any_cargo (player, GOOD_NUMBER (parameter))) &&
          GOOD_NUMBER (parameter) != SCRAP)
        break;
      if (good_prices[parameter] < goods[GOOD_NUMBER (parameter)].basic_value)
        break;
      fprintf (fd, "<OPTION VALUE=%d>", site);
      fprintf (fd, "Sell 1 %s to %s Colony for $%d\n",
               goods[GOOD_NUMBER (parameter)].name,
               races[RACE_NUMBER (parameter)].name, good_prices[parameter]);
      break;
    case factory:
      for (level = 0; level < 4; level++)
        {
          fprintf (fd, "<OPTION VALUE=%d%c>", site, 'a' + (1 << level) - 1);
          fprintf (fd, "Sell %d Scrap for $%d\n",
                   1 << level, 25 * (1 << level));
        }
      for (level = 0; level < 3; level++)
        {
          fprintf (fd, "<OPTION VALUE=%d%c>", site, 'A' + (1 << level));
          fprintf (fd, "Buy %d %s for $%d\n",
                   1 << level,
                   goods[GOOD_NUMBER (parameter)].name,
                   goods[GOOD_NUMBER (parameter)].basic_value * (1 << level));
        }
      break;
    case hall:
      for (skill = engineering; skill <= weaponry; skill++)
        {
          if (skill_level (player->skills[skill]) > player->crew[skill])
            fprintf (fd, "<OPTION VALUE=%d%c>Recruit %s crew\n",
                     site, skill + 'A', skill_names[skill]);
        }
      break;
    case prison:
      if (player->prisoner)
        {
          fprintf (fd, "<OPTION VALUE=%d>", site);
          fprintf (fd, "Cash in prisoner %s for $%d\n",
                   criminal_string (player->prisoner),
                   500 * (player->prisoner & 7) * (player->prisoner & 7));
        }
      break;
    default:
      break;
    }
}

void
show_location (FILE * fd, struct PLAYER *player, int site)
{
  location_sort sort = locations[site].sort;
  byte parameter = locations[site].parameter;
  int risk = locations[site].risk;
  int owner = locations[site].voter;

  if (sort != none)
    fprintf (fd, "<TR><TD>%d</TD><TD>", site);

  switch (sort)
    {
    case none:
      break;
    case academy:
      fprintf (fd, "%s Academy", skill_names[parameter]);
      break;
    case arsenal:
      fprintf (fd, "Arsenal");
      break;
    case belt:
      fprintf (fd, "Asteroid belt number %d", parameter);
      break;
    case badland:
      fprintf (fd, "Badland, danger %d%%", risk);
      break;
    case colony:
      fprintf (fd,
               "%s Colony buying %s for $%d (%d%%)<br>%d %s vote (%s has %d influence)",
               races[RACE_NUMBER (parameter)].name,
               goods[GOOD_NUMBER (parameter)].name, good_prices[parameter],
               (good_prices[parameter] * 100) /
               goods[GOOD_NUMBER (parameter)].basic_value,
               locations[site].votes,
               skill_names[races[RACE_NUMBER (parameter)].religion],
               owner ? name_string (players[owner].name) : "Neutral",
               locations[site].influence);
      break;
    case comet:
      fprintf (fd, "Comet Cloud");
      break;
    case corona:
      fprintf (fd, "Stellar Coronasphere, danger %d%%", risk);
      break;
    case deep_space:
      fprintf (fd, "Deep Space");
      break;
    case homeworld:
      fprintf (fd, "%s Homeworld<br>%d %s votes (%s has %d influence)",
               races[parameter].name,
               locations[site].votes,
               skill_names[races[parameter].religion],
               owner ? name_string (players[owner].name) : "Neutral",
               locations[site].influence);
      break;
    case gas_giant:
      fprintf (fd, "Gas Giant, danger %d%%", risk);
      break;
    case factory:
      fprintf (fd, "Factory selling %s for $%d each",
               goods[GOOD_NUMBER (parameter)].name,
               goods[GOOD_NUMBER (parameter)].basic_value);
      break;
    case hall:
      fprintf (fd, "Hiring hall");
      break;
    case minefield:
      fprintf (fd, "Minefield, danger %d%%", risk);
      break;
    case moon:
      fprintf (fd, "Moon");
      break;
    case near_space:
      fprintf (fd, "Near Space");
      break;
    case ocean:
      fprintf (fd, "Ocean");
      break;
    case prison:
      fprintf (fd, "Prison");
      break;
    case ruins:
      fprintf (fd, "Ancient Ruins");
      break;
    case school:
      fprintf (fd, "%s School", skill_names[parameter]);
      break;
    case stargate:
      fprintf (fd, "Stargate to %s (Key %d)",
               star_names[parameter], (locations[site].star ^ parameter) & 7);
      break;
    case terminal:
      fprintf (fd, "Starnet Terminal #%d", parameter);
      break;
    default:
      fprintf (fd, "Unknown thing, probably a bug");
      break;
    }
  if (locations[site].ring & player->rings_seen)
    fprintf (fd, " (%s)", ring_string (locations[site].ring));
  if (locations[site].rogues)
    fprintf (fd, " (%s %s rogue band)",
             races[locations[site].rogues >> 3].name,
             skill_names[(locations[site].rogues >> 1) & 3]);
  if (locations[site].criminal && get_crim (player, locations[site].criminal))
    fprintf (fd, " [%s]\n", criminal_string (locations[site].criminal));
  else
    fprintf (fd, "\n");
}

void
starname_input (FILE * fd)
{
  fprintf (fd,
           "or by name from starmap, eg S#48 <INPUT NAME=\"j\" SIZE=15>\n");
}



int
sale_price (struct ITEM *item)
{
  int result = 25 * (1 << item->efficiency);

  if (item->flags & ITEM_DEMO)
    return (0);
  if (item->sort < pod)
    {
      result *= item->reliability;
      result /= 100;
    }
  if (item->sort == pod || (item->flags & ITEM_BROKEN))
    result /= 2;
  return (result);
}

void
show_selling_options (FILE * fd, struct PLAYER *player)
{
  struct ITEM *item = items + player->ship;

  while (item != items)
    {
      fprintf (fd, "<OPTION VALUE=-%d>", item - items);
      fprintf (fd, "Sell %s %s (%d%%) for $%d\n",
               tech_level_names[item->efficiency],
               item_string (item), item->reliability, sale_price (item));
      item = items + item->link;
    }
}

void
show_system_options (FILE * fd, struct PLAYER *player, int star)
{
  int i;

  fprintf (fd, "<TD><SELECT NAME=\"c\" SIZE=6 MULTIPLE>\n");
  fprintf (fd, "<OPTION VALUE=\"\">Do Nothing\n");
  show_selling_options (fd, player);
  for (i = 0; i < MAX_LOCATION; i++)
    if (locations[i].star == star)
      show_location_option (fd, player, i);
  for (i = 0; i < MAX_SHOP; i++)
    if (shops[i].star == star)
      show_shop_options (fd, i);
  fprintf (fd, "</SELECT></TD>\n");
}

void
show_general_options (FILE * fd, struct PLAYER *player)
{
  int i, p;

  fprintf (fd, "<Input Name=\"n\" TYPE=HIDDEN VALUE=\"%s\">\n", player->name);
  fprintf (fd, "<Input Name=\"z\" VALUE=\"%d\" TYPE=HIDDEN>\n", turn + 1);
  fprintf (fd, "<Input Name=\"k\" VALUE=\"%d\" TYPE=HIDDEN>\n",
           make_key (player->name, turn + 1));
  fprintf (fd, "<Input Name=\"Z\" VALUE=\"%d\" TYPE=HIDDEN>\n", game);

  if (player->popcorn)
    {
      fprintf (fd, "</TR><TR ALIGN=CENTER>\n");
      fprintf (fd, "<TD><SELECT NAME=\"s\" SIZE=6 MULTIPLE>\n");
      i = 0;
      while (i <= player->popcorn)
        {
          fprintf (fd, "<OPTION VALUE=%d>Sell %d Popcorn\n", i, i);
          if (i)
            i *= 2;
          else
            i = 1;
        }
      fprintf (fd, "</SELECT></TD>\n");
    }

  fprintf (fd, "</TR><TR><TD>Buy <INPUT TYPE=TEXT SIZE=6 NAME=\"N\">");
  fprintf (fd, "popcorn spending up to $<INPUT TYPE=TEXT SIZE=6 NAME=\"O\">");
  fprintf (fd, " each</TD>\n");
  
  if (player->star >= MAX_STAR)
    {
      fprintf (fd,
               "</TR><TR ALIGN=CENTER><TD COLSPAN=3>WARNING - You are on holiday: submitting any orders will automatically return you to normal space!  And most orders won't run anyway.</TD>\n");
    }
  fprintf (fd, "</TR><TR ALIGN=CENTER>\n");

  fprintf (fd, "<TD><STRONG><INPUT TYPE=submit VALUE=\"Make It So\">\n");
  fprintf (fd, "<INPUT TYPE=reset></STRONG>\n");

  if (player->star != HOLIDAY && player->star < MAX_STAR)
    {
      fprintf (fd, "<SELECT NAME=\"e\">\n");
      for (i = 0; i < MAX_LOCATION; i++)
        if (locations[i].star == player->star &&
            loc_type (i, LOC_EXPLORABLE) && loc_accessible (player, i))
          fprintf (fd, "<OPTION VALUE=%d>Explore %s (%d)\n",
                   i, loc_string (i), i);
      fprintf (fd, "</SELECT>\n");
    }
  fprintf (fd, "</TD>\n");

  fprintf (fd, "</TR><TR ALIGN=CENTER>\n");

  fprintf (fd, "<TD><SELECT NAME=\"l\" SIZE=6>\n");
  fprintf (fd, "<OPTION VALUE=\"\">No Change\n");
  if (player->companion != 0 && player->star != HOLIDAY
      && player->star < MAX_STAR)
    {
      fprintf (fd, "<OPTION VALUE=%d>Un-ally with %s\n",
               BIG_NUMBER, name_string (players[player->companion].name));
      if (player->energy / 10 > 0 &&
          players[player->companion].star == player->star)
        fprintf (fd, "<OPTION VALUE=%d>Give %s $%d\n",
                 BIG_NUMBER + player->energy / 10,
                 name_string (players[player->companion].name),
                 player->energy / 10);
      if (player->energy / 20 > 0 &&
          players[player->companion].star == player->star)
        fprintf (fd, "<OPTION VALUE=%d>Give %s $%d\n",
                 BIG_NUMBER + player->energy / 20,
                 name_string (players[player->companion].name),
                 player->energy / 20);
      if (player->energy / 50 > 0 &&
          players[player->companion].star == player->star)
        fprintf (fd, "<OPTION VALUE=%d>Give %s $%d\n",
                 BIG_NUMBER + player->energy / 50,
                 name_string (players[player->companion].name),
                 player->energy / 50);
      if (player->energy / 100 > 0 &&
          players[player->companion].star == player->star)
        fprintf (fd, "<OPTION VALUE=%d>Give %s $%d\n",
                 BIG_NUMBER + player->energy / 100,
                 name_string (players[player->companion].name),
                 player->energy / 100);
    }
  for (p = 0; p < MAX_PLAYER; p++)
    if (p != (player - players) &&
        p != player->companion && player->star == players[p].star)
      fprintf (fd, "<OPTION VALUE=%d>Ally with %s\n",
               p, name_string (players[p].name));
  fprintf (fd, "</SELECT></TD></TR>\n");
}

void
show_starsystem (FILE * fd, struct PLAYER *player, int star)
{
  int i;

  if (star >= MAX_STAR)
    {
      char starname[512];
      snprintf(starname, 512, "%s Planet for %s",
               mothballed (player - players) ? "Mothball" : "Holiday",
               name_string(player->name));
      fprintf (fd,
               "<A NAME=\"%s\"></A><H1>Sector 0/0, Star system %s (Empty terrain):</H1>",
               starname, starname);
      fprintf (fd, "<H2>Locations in this system</H2>\n");
      fprintf (fd, "<TABLE BORDER=1><TR><TH>Id</TH><TH>Description</TH></TR>\n");
      fprintf (fd, "</TABLE>");
      show_other_ships(fd, player, star);
      return;
    }

  fprintf (fd,
           "<A NAME=\"%s\"></A><H1>Sector %02d/%02d, Star system %s (%s terrain; default range %s)",
           star_names[star], stars[star].x, stars[star].y, star_names[star],
           terrain_names[stars[star].terrain],
           range_names[stars[star].terrain]);

  if (stars[star].hidden && player->star != star)
    {
      fprintf (fd, " </H1>starsystem hidden from remote sensing this turn\n");
      return;
    }

  if (who_home (star) != NOT_HOMEWORLD)
    fprintf (fd, " Plague at %d%%:</H1>\n", races[who_home (star)].plague);
  else
    fprintf (fd, ":</H1>\n");
  fprintf (fd, "<H2>Locations in this system</H2>\n");
  fprintf (fd, "<TABLE BORDER=1><TR><TH>Id</TH><TH>Description</TH></TR>\n");
  for (i = 0; i < MAX_LOCATION; i++)
    if (locations[i].star == star)
      show_location (fd, player, i);
  show_adventures(fd,player);
  fprintf (fd, "</TABLE>\n");
  if (star == popcorn.star)
    {
      fprintf (fd,
               "<TABLE BORDER=1><TR><TH>Popcorn Source: Impulse %d%%, Sensor %d%%, Shield %d%%</TH></TR></TABLE>",
               popcorn.impulse_limit, popcorn.sensor_limit,
               popcorn.shield_limit);
    }
  show_other_ships (fd, player, star);
}

void
show_gif_map (FILE * fd, struct PLAYER *player)
{
  int x, y;
  char name[80];
  int skipping;
  int jump;

  fprintf (fd, "<HR><H2>Starmap</H2>\n");
  if ((player->star >= 0 && player->star < MAX_STAR)
      || player->star == OLYMPUS)
    {
      fprintf (fd, "<p id=\"jumpcost\">jump cost = 0</p>\n");
    }

  fprintf (fd, "<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0>\n");
  for (y = 0; y < MAXY; y++)
    {
      fprintf (fd, "<TR ALIGN=CENTER>\n");
      skipping = 0;
      for (x = 0; x < MAXX; x++)
        {
          if (board[x][y] != 0xff && star_seen (player, board[x][y]))
            {
              if (skipping)     /* catch up */
                fprintf (fd, "<TD COLSPAN=%d> </TD>\n", skipping);
              if (board[x][y] >= MAX_HAB_STAR)
                {
                  fprintf (fd, "<TD><IMG SRC=\"nonhab.gif\"");
                  sprintf (name, "S#%-02d", board[x][y] - MAX_HAB_STAR);
                }
              else
                {
                  fprintf (fd, "<TD><IMG SRC=\"hab.gif\"");
                  sprintf (name, "%s", star_names[board[x][y]]);
                }
              if ((player->star >= 0 && player->star < MAX_STAR)
                  || player->star == OLYMPUS)
                {
                  fprintf (fd, " onmouseover=\"document.getElementById");
                  fprintf (fd, "('jumpcost').textContent = ");
                  jump = jump_cost(player - players,
                                   player->star, board[x][y]);
                  if (jump <= player->energy)
                    {
                      fprintf (fd, "'jump cost = %d';\"", jump);
                    }
                  else
                    {
                      fprintf (fd, "'jump cost = too much';\"");
                    }
                }
              else
                {
                  /* printf("no mapulator from star %d\n",
                     player->star); */
                }

              fprintf(fd, "></TD>");
              name[NAME_SIZE] = '\0';
              fprintf (fd, "<TD COLSPAN=%d>%s</TD>", strlen (name), name);
              x += NAME_SIZE - 1;
              skipping = NAME_SIZE - strlen (name) - 1;
            }
          else
            skipping++;
        }
      if (skipping)             /* catch up */
        fprintf (fd, "<TD COLSPAN=%d> </TD>\n", skipping);
      fprintf (fd, "</TR>\n");
    }
  fprintf (fd, "</TABLE><HR>\n");
  if (player->last_restart == turn)
    fprintf (fd, "<p>Run Long Range Scan to populate starmap</p>");
}

void
starnet_report (struct PLAYER *player)
{
  FILE *fd;
  char buffer[80];
  int loc;

  sprintf (buffer, "%s/results/%d/report_%d_%d.html",
           webroot, game, player - players, turn);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't open report file\n");
      exit (1);
    }
  fprintf (fd, "<HTML><HEAD><TITLE>Starnet Report, Turn %d</TITLE>\n", turn);
  html_header (fd, player->web_source);
  fprintf (fd, "<H1>Starnet Report, Turn %d</H1>\n", turn);
  for (loc = 0; loc < MAX_LOCATION; loc++)
    {
      if (locations[loc].sort == terminal
          && (player->reports & (1 << locations[loc].parameter))
          && (star_seen (player, locations[loc].star)))
        show_starsystem (fd, player, locations[loc].star);
    }
  fprintf(fd, "</center></body></html>\n");
  fclose (fd);
}

const char *
password (int key)
{
  int value;

  value = key & 1 ? password_true & 0xff : password_false & 0xff;
  value |= key & 2 ? password_true & 0xff00 : password_false & 0xff00;
  value |= key & 4 ? password_true & 0xff0000 : password_false & 0xff0000;
  value |= key & 8 ? password_true & 0xff000000 : password_false & 0xff000000;
  return (uint32_name (value));
}

void
generate_options (FILE * fd, struct PLAYER *player, skill_sort sort)
{
  struct ITEM *item = items + player->ship;
  int i, loc, level, parameter, criminal;
  int done_popcorn = FALSE, done_medicine = FALSE;

  if (sort == engineering && total_item (warp_drive, items + player->ship) == 0 &&
      player->energy >= 500)
    {
      fprintf (fd,
               "<OPTION VALUE=H1>Buy a primitive demo warp drive for $500\n");
    }

  if (player->chosen & (1 << sort))
    {
      fprintf (fd, "<OPTION VALUE=L%d>Commune with %s\n",
               -(sort + 1), god_names[sort]);
    }

  if (sort == medical)
    {
      fprintf (fd, "<OPTION VALUE=H0>Heal Crew\n");
      if (who_home (player->star) != NOT_HOMEWORLD
          && effective_skill_level (player, medical) > 0
          && factor (sick_bay, player) > 0)
        fprintf (fd, "<OPTION VALUE=K>Cure %s Plague\n",
                 races[who_home (player->star)].name);
    }
  if (sort == science)
    fprintf (fd, "<OPTION VALUE=L-5>Long Range Scan\n");
  if (sort == weaponry)
    {
      if (player->crew[weaponry])
        {
          for (loc = 0; loc < MAX_LOCATION; loc++)
            {
              if (locations[loc].star != player->star)
                continue;
              criminal = locations[loc].criminal;
              if (criminal && get_crim (player, criminal))
                /* spotted crook */
                fprintf (fd, "<OPTION VALUE=G%d>Capture %s\n",
                         loc, criminal_string (criminal));
            }
        }
      if (player->prisoner)
        {
          fprintf (fd, "<OPTION VALUE=I0>Interrogate %s\n",
                   criminal_string (player->prisoner));
          fprintf (fd, "<OPTION VALUE=I1>Release %s for bribe ($%d)\n",
                   criminal_string (player->prisoner),
                   500 * (player->prisoner & 7) * (player->prisoner & 7));
          fprintf (fd, "<OPTION VALUE=I2>Release %s for information\n",
                   criminal_string (player->prisoner));
        }
    }
  if (player->crew[sort])
    fprintf (fd, "<OPTION VALUE=T%d>Train Crew\n", sort);
  fprintf (fd, "<OPTION VALUE=m>Priority Maintenance\n");
  while (item - items)
    {
      if (repairers[item->sort] == sort && item->sort < pod)
        {
          if (item->flags & ITEM_BROKEN)        /* broken */
            fprintf (fd, "<OPTION VALUE=R%d>Repair %s\n",
                     item - items, item_string (item));
          if (effective_skill_level (player, sort) >
              item->efficiency * item->efficiency
              && !(item->flags & ITEM_DEMO) && item->reliability < 99)
            fprintf (fd, "<OPTION VALUE=M%d>Maintain %s (%d%%)\n",
                     item - items, item_string (item), item->reliability);
        }
      item = items + item->link;
    }
  for (loc = 0; loc < MAX_ADVENTURE; loc++)
    {
      parameter = adventures[loc].parameter;
      if (adventures[loc].star == player->star &&
          get_ad (player, parameter) &&
          effective_skill_level (player,
                                 ADVENTURE_SKILL (parameter)) >=
          ADVENTURE_LEVEL (parameter))
        {
          if (ADVENTURE_SKILL (parameter) == sort)
            fprintf (fd, "<OPTION VALUE=A%d>Lead %s (%s%d)\n", loc,
                     ad_types[ADVENTURE_TYPE (parameter)].ad_name,
                     skill_names[ADVENTURE_SKILL (parameter)],
                     ADVENTURE_LEVEL (parameter));
          else                  /* helper officers */
            fprintf (fd, "<OPTION VALUE=A%d>Help %s (%s%d)\n", loc,
                     ad_types[ADVENTURE_TYPE (parameter)].ad_name,
                     skill_names[ADVENTURE_SKILL (parameter)],
                     ADVENTURE_LEVEL (parameter));
        }
    }
  switch(sort)
    {
    case engineering:
      fprintf (fd, "<OPTION VALUE=U-1>Induce sunspots\n");
      break;
    case science:
      fprintf (fd, "<OPTION VALUE=U-2>Induce stellar flares\n");
      break;
    case medical:
      fprintf (fd, "<OPTION VALUE=U-3>Feed stellar amoebae\n");
      break;
    case weaponry:
      fprintf (fd, "<OPTION VALUE=U-4>Shoot star\n");
      break;
    }
  for (loc = 0; loc < MAX_LOCATION; loc++)
    {
      int type = locations[loc].sort;
      if (locations[loc].star != player->star ||
          (!loc_accessible (player, loc)))
        continue;

      // Added test for "none".
      if ((location_types[type].instability_skill == sort) && !strcmp(location_types[type].name, "none" ))
        {
          switch(sort)
            {
            case engineering:
              if (effective_skill_level (player, engineering) > 0)
                 fprintf (fd, "<OPTION VALUE=U%d>Destabilize orbit of %s %d\n",
                          loc, location_types[type].name, loc);
              break;
            case science:
              if (effective_skill_level (player, science) > 0)
                fprintf (fd, "<OPTION VALUE=U%d>Dephase %s %d\n",
                         loc, location_types[type].name, loc);
              break;
            case medical:
              if (effective_skill_level (player, science) > 0)
                fprintf (fd, "<OPTION VALUE=U%d>Poison %s %d\n",
                         loc, location_types[type].name, loc);
              break;
            case weaponry:
              if (effective_skill_level (player, weaponry) > 0)
                 fprintf (fd, "<OPTION VALUE=U%d>Sabotage %s %d\n",
                          loc, location_types[type].name, loc);
              break;
            }
        }

      if (player->rings_seen & locations[loc].ring & (1 << sort))
        {
          fprintf (fd, "<OPTION VALUE=W%d>Collect %s\n",
                   loc, ring_string (locations[loc].ring));
        }

      parameter = locations[loc].parameter;
      switch (locations[loc].sort)
        {
        case academy:
          if (parameter == sort)
            for (level = 1;
                 level <= skill_level (player->skills[sort]) / 4 + 1; level++)
              if ((player->
                   skills[parameter] & skill_bit (academy_skill, level)) == 0)
                fprintf (fd, "<OPTION VALUE=%d%c>Academy Level %d ($%d)\n",
                         loc, 'A' + level, level, level * level * 100);
          break;
        case badland:
        case gas_giant:
          if (sort == ((locations[loc].rogues >> 1) & 3)
              && locations[loc].rogues)
            fprintf (fd, "<OPTION VALUE=%d>Recruit rogue band (%d%%)\n", loc,
                     locations[loc].risk);
          break;
        case belt:
          if (sort == engineering)
            fprintf (fd, "<OPTION VALUE=%d>Mine asteroids\n", loc);
          break;
        case comet:
          if (sort == engineering)
            fprintf (fd, "<OPTION VALUE=%d>Harvest Chocolate\n", loc);
          break;
        case corona:
          if (sort == engineering)
            fprintf (fd, "<OPTION VALUE=%d>Skim Star for energy\n", loc);
          break;
        case deep_space:
        case near_space:
          if (sort == science && player->star == popcorn.star
              && !done_popcorn)
            {
              done_popcorn++;
              fprintf (fd, "<OPTION VALUE=%d>Collect Popcorn\n", loc);
            }
          break;
        case colony:
          if (races[RACE_NUMBER (parameter)].religion == sort)
            fprintf (fd, "<OPTION VALUE=%d>Influence %s Colony\n",
                     loc + BIG_NUMBER, races[RACE_NUMBER (parameter)].name);
          break;
        case homeworld:
          if (races[parameter].religion == sort)
            fprintf (fd, "<OPTION VALUE=%d>Influence %s Homeworld\n",
                     loc + BIG_NUMBER, races[parameter].name);
          if (sort == weaponry && player->rank == 1)
            fprintf (fd, "<OPTION VALUE=%d>Intimidate %s Government\n", loc,
                     races[parameter].name);
          break;
        case minefield:
          if (sort == weaponry)
            fprintf (fd, "<OPTION VALUE=%d>Salvage photon torpedoes\n", loc);
          break;
        case ocean:
          if (done_medicine++)
            break;
          if (sort == medical)
            fprintf (fd, "<OPTION VALUE=%d>Research Medicine\n", loc);
          break;
        case school:
          if (parameter == sort)
            {
              if ((player->skills[parameter] & skill_bit (school_skill, 1)) ==
                  0)
                fprintf (fd, "<OPTION VALUE=%dB>School Level 1 ($50)\n", loc);
              if ((player->skills[parameter] & skill_bit (school_skill, 2)) ==
                  0)
                fprintf (fd, "<OPTION VALUE=%dC>School Level 2 ($100)\n",
                         loc);
            }
          break;
        case terminal:
          if (sort == science)
            {
              if ((player->
                   experience[science] & (1 << locations[loc].parameter)) ==
                  0)
                fprintf (fd, "<OPTION VALUE=%d>Access Starnet\n", loc);
            }
          break;
        default:
          break;
        }
    }
  loc = star_has_loc (player->star, terminal);
  if (loc != NO_LOCATION && (player->experience[science] & (1 << loc)))
    for (i = 0; i < 4; i++)
      fprintf (fd, "<OPTION VALUE=V%d>Use Password %s\n",
               i + sort * 4, password (i + sort * 4));
  generate_evil_options (fd, player, sort);
}

int
star_has_ring (int star, skill_sort sort)
{
  int loc;

  for (loc = 1; loc < MAX_LOCATION; loc++)
    if (locations[loc].star == star && (locations[loc].ring & (1 << sort)))
      return (loc);
  return (FALSE);
}


void
retire_prophet (skill_sort skill)
{
  int player;

  prophets[skill] = -1;
  for (player = 1; player < MAX_PLAYER; player++)
    {
      players[player].chosen &= ~(1 << skill);
      players[player].heretic &= ~(1 << skill);
    }
}




void
show_scrap_options (FILE * fd, struct PLAYER *player)
{
  struct ITEM *item;

  fprintf (fd, "<TD><SELECT NAME=\"b\" SIZE=6 MULTIPLE>\n");
  fprintf (fd, "<OPTION VALUE=\"\">Nothing\n");
  item = items + player->ship;
  while (item != items)
    {
      if (item->sort == pod && item->reliability > SCRAP)
        {
          if (item->reliability < BASE_UNIT)
            {
              fprintf (fd, "<OPTION VALUE=-%d>Scrap %d %s\n",
                       item - items, item->collection,
                       goods[item->reliability].name);
            }
          else
            {
              fprintf (fd, "<OPTION VALUE=-%d>Demob %s\n",
                       item - items, units[item->reliability - BASE_UNIT].name);
            }

        }
      item = items + item->link;
    }
  item = items + player->ship;
  while (item != items)
    {
      if ((!(item->flags & ITEM_DEMO)) && (item->sort != pod))
        fprintf (fd, "<OPTION VALUE=%d>Scrap %s\n",
                 item - items, item_string (item));
      item = items + item->link;
    }
  fprintf (fd, "</SELECT></TD>\n");

  fprintf (fd, "</TR>\n<TR>\n");
  
  fprintf (fd, "<TD ALIGN=\"CENTER\"><SELECT NAME=\"D\" SIZE=6 MULTIPLE>\n");
  fprintf (fd, "<OPTION VALUE=\"\">Nothing\n");
  item = items + player->ship;

  while (item != items)
    {
      /* Populating the list of items that can be shut down.
         We want to remove Demo Mods from this list.
         If we're wrong then *only* demo mods will be listed. - cb */
      //if (item->sort < pod && !(item->flags & ITEM_BROKEN) )
      if (item->sort < pod && !(item->flags & ITEM_BROKEN) && !(item->flags & ITEM_DEMO))
        fprintf (fd, "<OPTION VALUE=%d>Shut down %s\n",
                 item - items, item_string (item));
      item = items + item->link;
    }
  fprintf (fd, "</SELECT></TD>\n");
}

void
show_merc_options (FILE * fd, struct PLAYER *player)
{
  int bid;

  fprintf (fd, "<HR><H2>");
  print_rules_link(fd, "Mercenaries", "Mercenary");
  fprintf (fd, " Actions</H2>\n");
  if ((player->flags & DISGRACED) && (dice (20) == 0))
    {
      printf ("%s redeemed\n", player->name);
      player->flags &= ~DISGRACED;
      fprintf (fd, "<P>Mercenaries are now willing to work for you again\n");
    }
  if ((next_unit == -1) || (player->flags & DISGRACED))
    fprintf (fd, "<P>No unit available for hire this turn\n");
  else
    {
      fprintf (fd, "<P>Next unit available for hire is the %s\n",
               units[next_unit].name);
      fprintf (fd,
               "<BR>To hire them, use either or both of these menus to bid salary you'll pay them PER TURN:\n");
      fprintf (fd, "<SELECT NAME=\"u\">\n");
      for (bid = 0; bid < 100; bid += 10)
        {
          fprintf (fd, "<OPTION VALUE=-%d>Pay $%d per turn\n", bid, bid);
        }
      fprintf (fd, "</SELECT>\n");
      fprintf (fd, "<SELECT NAME=\"u\">\n");
      for (bid = 0; bid < 10; bid++)
        {
          fprintf (fd, "<OPTION VALUE=-%d>Pay $%d per turn\n", bid, bid);
        }
      fprintf (fd, "</SELECT><P>\n");
    }


  fprintf (fd, "<P>A contract for next turn is offered at %s\n",
           star_names[locations[next_contract].star]);
  fprintf (fd, "<BR>The terrain is %s, fee is $%d\n",
           loc_string (next_contract), next_fee);
  fprintf (fd,
           "<BR>To win it, be at %s at the end of next turn with the strongest mercenary force, allowing for terrain bonuses/penalties\n",
           star_names[locations[next_contract].star]);
}

void
show_orders (FILE *fd, struct PLAYER *player)
{
  FILE *temp;
  char buffer[256];
  skill_sort sort;
  double r1, r2;
  int energy = player->energy;
  struct PLAYER *enemy;

  enemy = escorted (player) ? 0 : pairing (player);
  
  /* check whether player has ever tested direct mail OK */
  sprintf (buffer, "%s/orders/%d/%s0", webroot, game, player->name);
  temp = fopen (buffer, "r");
  if (!temp)
    player->preferences &= ~4;
  if (strstr (player->address, "webtv"))
    {
      player->preferences |= 130;
    }

  fprintf(fd, "<!-- Order submission begins -->\n");
  fprintf(fd, "Your information-sharing url is: share_%s.htm<br>\n",
          uint32_name(public_password(player->password)));
  fprintf (fd,
           "<FORM ACTION=\"http://%s/cgi-bin/submit.cgi\" METHOD=\"POST\">\n",
           server);
  //      if (player->star != HOLIDAY && player->star < MAX_STAR)
  {
    generate_voting_options (fd, player);
    generate_presidential_options (fd, player);
    generate_prophet_options (fd, player);
  }
  fprintf (fd, "<TABLE BORDER=1>\n");
  for (sort = engineering; sort <= weaponry; sort++)
    {
      fprintf (fd, "<TR><TH COLSPAN=4>%s (Skill %d)</TH></TR>\n",
               skill_names[sort], effective_skill_level (player, sort));
      fprintf (fd, "<TR ALIGN=CENTER><TD COLSPAN=2><SELECT NAME=\"%c\">\n",
               skill_names[sort][0]);
      fprintf (fd, "<OPTION VALUE=.>Stand by\n");
      generate_options (fd, player, sort);
      fprintf (fd, "</SELECT></TD>\n");
      fprintf (fd, "<TD COLSPAN=2><SELECT NAME=\"x\">\n");
      fprintf (fd, "<OPTION VALUE=.>No Spells\n");
      generate_magic_options (fd, player, sort, enemy);
      fprintf (fd, "</SELECT></TD></TR>\n");
    }
  fprintf (fd, "<TR><TH COLSPAN=4>Jump To (use one menu only):</TH></TR>\n");
  fprintf (fd, "<TR ALIGN=CENTER><TH>Short</TH><TH>Medium</TH><TH>Long</TH><TH>Impossible</TH></TR>\n");
  fprintf (fd, "<TR ALIGN=CENTER><TD>\n");
  r1 = 0;
  r2 = inverse_jump_cost(player-players, 100);
  show_destinations (fd, player - players, player->star, r1, r2, 1);
  fprintf (fd, "</TD><TD>\n");
  r1 = r2;
  r2 = inverse_jump_cost(player-players, 1000);
  show_destinations (fd, player - players, player->star, r1, r2, 1);
  fprintf (fd, "</TD><TD>\n");
  r1 = r2;
  r2 = inverse_jump_cost(player-players, energy);
  show_destinations (fd, player - players, player->star, r1, r2, 1);
  fprintf (fd, "</TD><TD>\n");
  r1 = r2;
  r2 = BIG_NUMBER;
  show_destinations (fd, player - players, player->star, r1, r2, 0);
  fprintf (fd, "</TD></TR>\n");
  fprintf (fd, "</TABLE>\n");
  fprintf (fd, "<TABLE BORDER=1>\n");
  fprintf (fd, "<TR><TH ALIGN=CENTER>Minor Options<div style=\"font-size:8pt;\">Hold control or command key to select multiple options.</div></TH></TR>\n");
  fprintf (fd, "<TR ALIGN=CENTER>\n");
  show_scrap_options (fd, player);
  fprintf (fd, "</TR><TR ALIGN=CENTER>\n");
  show_system_options (fd, player, player->star);
  show_general_options (fd, player);
  fprintf (fd, "</TABLE>\n");
  if (enemy && player->star != HOLIDAY && enemy != dybuk)
    show_combat_options (fd, player, enemy);
  show_merc_options (fd, player);
  fprintf (fd, "<HR><H2>Flag to Display</H2>\n");
  fprintf (fd, "<INPUT TYPE=TEXT SIZE=70 NAME=\"f\" VALUE=\"%s\">",
           player->banner_source);
  fprintf (fd,
           "<br>(No HTML allowed. Images can be accessed as a special case, but only from the %s server's <a href=\"http://%s/images\">images</a> directory, uploaded via your w++ account page. To set one of these images as your banner, use the hash character and its filename, eg #deathstar.gif)\n",
           server, server);
  fprintf (fd, "<H2>Press/Rumours for next issue of ");
  fprintf (fd, "<A HREF=\"http://%s/times.html\">Subspace Times</A></H2>\n",
           server);
  fprintf (fd, "<INPUT TYPE=TEXT SIZE=70 NAME=\"p\"><BR>Signed %s<BR>\n",
           player->address);
  fprintf (fd, "<INPUT TYPE=TEXT SIZE=70 NAME=\"q\"><BR>Signed %s<BR>\n",
           name_string (player->name));
  fprintf (fd,
           "<INPUT TYPE=TEXT SIZE=70 NAME=\"o\"><BR>Signed Anonymous<BR>\n");

  fprintf (fd, "<HR>\n");
  fprintf (fd,
           "<H2>Links to Useful Resources - Don't Die of Ignorance!</H2>\n");
  fprintf (fd, "</CENTER>\n");
  fprintf (fd,
           "<LI>The <A HREF=\"http://%s/Rules.html\"><EM>RULES</EM></A>: if you want to do well, read early and often\n",
           server);
  fprintf (fd,
           "<LI>To contact other players you can use these <A HREF=\"http://%s/alias.html\">utilities</A> for sending anonymous mail to other players and other odd services. Remember to sign mail if you want a reply.\n",
           server);
  fprintf (fd,
           "<LI>Keep up with news, gossip, and libel with the latest issue of the <A HREF=\"http://%s/times.html\">Subspace Times</A>, \n",
           server);
  fprintf (fd,
           "or browse back copies via the <A HREF=\"http://%s/news/times.cgi\">SST Times archive</A>\n",
           server);
  fprintf (fd,
           "<LI>Some players have <A HREF=\"http://%s/players\">pages</A> on the TBG server. Note that while the bigger ones are generally more interesting, some of the small ones are just links to interesting TBG pages on other servers.",
           server);
  fprintf (fd,
           "<LI>Use <A HREF=\"http://%s/open.html\">this form</A> to open a w++ account. For TBG this gives you a web page on %s linked to your shipname and a way to put images in your banner.\n",
           server, server, server);
  fprintf (fd,
           "<LI>There's a mailing list for design discussion on TBG and our other games, hosted by <A HREF=\"http://groups.yahoo.com/subscribe/TBG_Design\">Yahoo groups</A>. To join, register with Yahoo and then subscribe to TBG_Design\n");
  fprintf (fd,
           "<LI>For wider discussions, you can also check out the TBG <A HREF=\"http://groups.yahoo.com/subscribe/TBG_OpenForum\">Open Forum</A>. Also provided by yahoo groups.\n");
  fprintf (fd,
           "<LI>Here are just the <a href=\"http://%s/news/rank.cgi?%s\">ranking lists</a> which contain your ship\n",
           server, url_shipname (player->name));
  fprintf (fd,
           "<CENTER><P><HR><H2>Administrative Section (Not needed in normal turns)</H2>\n");
  if (player->x_from[0])
    fprintf (fd, "Your orders this turn came from %s.\n", player->x_from);
  fprintf (fd, "<H2>Preferences</H2>\n");
  fprintf (fd, "<SELECT NAME=\"u\">\n");
  fprintf (fd, "<OPTION VALUE=0 %s>Separate Times Mailing\n",
           player->preferences & 1 ? "" : "SELECTED");
  fprintf (fd, "<OPTION VALUE=1 %s>Results & Times in Same Mailing\n",
           player->preferences & 1 ? "SELECTED" : "");
  fprintf (fd, "</SELECT>\n");

  fprintf (fd, "<SELECT NAME=\"u\">\n");
  fprintf (fd, "<OPTION VALUE=0 %s>Mail via server\n",
           player->preferences & 4 ? "" : "SELECTED");
  fprintf (fd, "<OPTION VALUE=4 %s>Mail directly\n",
           player->preferences & 4 ? "SELECTED" : "");
  fprintf (fd, "</SELECT>\n");

  fprintf (fd, "<SELECT NAME=\"u\">\n");
  fprintf (fd, "<OPTION VALUE=0 %s>Restart on loss of ship\n",
           player->preferences & 8 ? "" : "SELECTED");
  fprintf (fd, "<OPTION VALUE=8 %s>No automatic restart\n",
           player->preferences & 8 ? "SELECTED" : "");
  fprintf (fd, "</SELECT>\n");

  fprintf (fd, "<SELECT NAME=\"u\">\n");
  fprintf (fd, "<OPTION VALUE=0 %s>Don't Acknowledge Orders\n",
           player->preferences & 16 ? "" : "SELECTED");
  fprintf (fd, "<OPTION VALUE=16 %s>Acknowledge Orders\n",
           player->preferences & 16 ? "SELECTED" : "");
  fprintf (fd, "</SELECT>\n");

  fprintf (fd, "<SELECT NAME=\"u\">\n");
  fprintf (fd, "<OPTION VALUE=0 %s>Keep secret URL %s\n",
           "SELECTED", uint32_name (player->password));
  fprintf (fd, "<OPTION VALUE=64>Change to new secret URL\n");
  fprintf (fd, "</SELECT>\n");

  fprintf (fd, "<SELECT NAME=\"u\">\n");
  fprintf (fd, "<OPTION VALUE=0 %s>Mail Full Results\n",
           player->preferences & 128 ? "" : "SELECTED");
  fprintf (fd, "<OPTION VALUE=128 %s>Mail Only Secret URL\n",
           player->preferences & 128 ? "SELECTED" : "");
  fprintf (fd, "</SELECT>\n");

  fprintf (fd, "<SELECT NAME=\"u\">\n");
  fprintf (fd, "<OPTION VALUE=0 %s>Minimise Ship Details\n",
           player->preferences & 256 ? "" : "SELECTED");
  fprintf (fd, "<OPTION VALUE=256 %s>Full Ship Details\n",
           player->preferences & 256 ? "SELECTED" : "");
  fprintf (fd, "</SELECT>\n");

  fprintf (fd, "<H2>New Web Root</H2>\n");
  fprintf (fd, "<INPUT TYPE=TEXT SIZE=70 NAME=\"w\" VALUE=\"%s\">",
           player->web_source);
  fprintf (fd, "</FORM>\n");

  if (player->preferences & 4)
    {
      fprintf (fd, "<FORM ACTION=\"mailto:tbg@%s\" METHOD=\"POST\">\n",
               mail_server);
    }
  else
    {
      fprintf (fd,
               "<FORM ACTION=\"http://%s/cgi-bin/submit.cgi\" METHOD=\"POST\">\n",
               server);
      fprintf (fd, "<INPUT TYPE=\"HIDDEN\" NAME=\"to\" VALUE=\"tbg@%s\">\n",
               mail_server);
    }
  fprintf (fd, "<Input Name=\"n\" TYPE=HIDDEN VALUE=\"%s\">\n", player->name);
  fprintf (fd, "<Input Name=\"z\" VALUE=\"%d\" TYPE=HIDDEN>\n", turn + 1);
  fprintf (fd, "<Input Name=\"y\" VALUE=\"1\" TYPE=HIDDEN>\n");
  fprintf (fd, "<Input Name=\"k\" VALUE=\"%d\" TYPE=HIDDEN>\n",
           make_key (player->name, turn + 1));
  fprintf (fd, "<Input Name=\"Z\" VALUE=\"1\" TYPE=HIDDEN>\n");
  fprintf (fd,
           "<INPUT TYPE=submit VALUE=\"New Position (ie discard current ship and crew completely!)\">");
  fprintf (fd, "</FORM>\n");

  if (player->preferences & 4)
    {
      fprintf (fd, "<FORM ACTION=\"mailto:tbg@%s\" METHOD=\"POST\">\n",
               mail_server);
    }
  else
    {
      fprintf (fd,
               "<FORM ACTION=\"http://%s/cgi-bin/submit.cgi\" METHOD=\"POST\">\n",
               server);
      fprintf (fd, "<INPUT TYPE=\"HIDDEN\" NAME=\"to\" VALUE=\"tbg@%s\">\n",
               mail_server);
    }
  fprintf (fd, "<Input Name=\"n\" TYPE=HIDDEN VALUE=\"%s\">\n", player->name);
  fprintf (fd, "<Input Name=\"z\" VALUE=\"%d\" TYPE=HIDDEN>\n", turn + 1);
  fprintf (fd, "<Input Name=\"k\" VALUE=\"%d\" TYPE=HIDDEN>\n",
           make_key (player->name, turn + 1));
  fprintf (fd, "<Input Name=\"Z\" VALUE=\"1\" TYPE=HIDDEN>\n");
  fprintf (fd, "<Input Name=\"y\" VALUE=\"-1\" TYPE=HIDDEN>\n");
  fprintf (fd,
           "<INPUT TYPE=submit VALUE=\"Drop Out (ie stop playing completely!)\">");
  fprintf (fd, "</FORM>\n");

  if (!temp)
    {
      fprintf (fd, "<Form method=POST action=\"mailto:tbg@%s\">\n", mail_server);
      fprintf (fd, "<Input Name=\"z\" VALUE=0 TYPE=HIDDEN>\n");
      fprintf (fd, "<Input Name=\"Z\" VALUE=1 TYPE=HIDDEN>\n");
      fprintf (fd, "<Input Name=\"k\" VALUE=\"%d\" TYPE=HIDDEN>\n",
               make_key (player->name, turn + 1));
      fprintf (fd, "<Input Name=\"n\" VALUE=\"%s\" TYPE=HIDDEN>\n",
               player->name);
      fprintf (fd, "<INPUT TYPE=submit VALUE=\"Direct Mail Test\">");
      fprintf (fd, "</FORM>\n");
    }
  else
    fclose (temp);
  fprintf(fd, "<!-- Order submission ends -->\n");
}

void
show_player (FILE * fd, struct PLAYER *player)
{
  if (player->star >= MAX_STAR)
    fprintf (fd,
             "<P>(ending turn at star system %s Planet for %s with $%d of energy and crew health of %d.%d%%\n",
             mothballed (player - players) ? "Mothball" : "Holiday",
             name_string (player->name), player->energy, player->health / 10,
             player->health % 10);
  else
    fprintf (fd,
           "<P>(ending turn at star system %s with $%d of energy and crew health of %d.%d%%\n",
           star_names[player->star], player->energy, player->health / 10,
           player->health % 10);
  show_orders(fd,player);
  fprintf (fd,
           "<hr><a href=\"http://%s\"><img src=\"http://%s/counter.gif\"></a>\n",
           server, server);

  fprintf (fd, "</CENTER></BODY></HTML>\n");
}


void
create_header (struct PLAYER *player)
{
  FILE *fd;
  char buffer[256];

  sprintf (buffer, "%s/results/%d/%s%d.h", webroot,
           game, player->name, turn);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create header file\n");
      exit (1);
    }
  fprintf (fd, "<HTML><HEAD><TITLE>To Boldly Go, Turn %d</TITLE>\n", turn);
  html_header (fd, player->web_source);
  fprintf (fd, "<H1>To Boldly Go - Starship %s, Turn %d</H1>\n",
           name_string (player->name), turn);
  if (player->rings_held)
    fprintf (fd, "Holding %s\n", ring_string (player->rings_held));
  if ((player->preferences & 32) == 0)
    {
      if (player->star >= MAX_STAR)
        fprintf (fd,
                 "(starting turn at star system %s Planet for %s with $%d of energy, playing since turn %d)<BR>\n",
                 mothballed (player - players) ? "Mothball" : "Holiday",
                 name_string (player->name), player->energy, player->last_restart);
      else
        fprintf (fd,
                 "(starting turn at star system %s with $%d of energy, playing since turn %d)<BR>\n",
                 star_names[player->star], player->energy, player->last_restart);
      }

  fclose (fd);
}


void
mothball_player (struct PLAYER *player)
{
  int i, p;
  if (ministers[PRESIDENT] == player - players)
    {
      ministers[PRESIDENT] = ministers[VEEP];
      ministers[VEEP] = 0;
    }
  for (i = 0; i < 9; i++)
    if (ministers[i] == player - players)
      ministers[i] = 0;
  for (i = 0; i < 4; i++)
    if (prophets[i] == player - players)
      retire_prophet (i);
  for (i = 0; i < MAX_LOCATION; i++)
    if (locations[i].voter == player - players)
      {
        locations[i].voter = 0;
        locations[i].influence = 0;
      }
  if (player->rings_held)
    {
      for (i = 0; i < MAX_RING; i++)
        if (player->rings_held & (1 << i))
          {
            init_ring (i);
          }
      player->rings_held = 0;
    }
  player->tracer = 0;
  player->probe = NOWHERE;
  player->companion = 0;
  for (p = 0; p < MAX_PLAYER; p++)
    {
      if (players[p].tracer == player - players)
        players[p].tracer = 0;
      if (players[p].companion == player - players)
        players[p].companion = 0;
    }
}

  

/* sort is 1 for a first restart, 2 for a subsequent one */
void
init_new_player (struct PLAYER *player, int sort)
{
  char buffer1[256], buffer2[256], buffer3[256];
  int try, other, ok, prefs;
  int passwd, account;
  int special = sort == 3;

  if (sort == 3)
    sort = 1;

  printf ("initializing player: %s\n", player->name);

  mothball_player (player);
  
  if (player->ship)
    destroy_ship (player);
  account = player->account_number;
  passwd = player->password;
  prefs = player->preferences;
  strcpy (buffer1, player->name);
  strcpy (buffer2, player->address);
  strcpy (buffer3, player->web_source);

  memset (player, 0, sizeof (*player));
  memcpy (player->stars, public_stars, (MAX_STAR / 32)*sizeof (uint32));
  player->preferences = 32;     /* keep restart in for testing */
  if (sort != 2)
    {
      player->preferences |= 128;
      /* goto web results for new players */
    }
  else
    player->preferences = prefs;
  player->restarting = sort;
  if ((sort == 255 || sort == -1 ) && really_send)
    account = 0;
  if (account == 0)
    {
      strcpy (buffer2, "nobody@localhost");
      player->account_number = 0;
    }
  else
    player->account_number = player - players;
  if (sort == 255)              /* dropout */
    {
      fprintf (report, "%s (%d) dropping out\n", buffer1, player - players);
      printf ("%s dropping out\n", buffer1);
      sort = 1;
    }
  strcpy (player->name, buffer1);
  strcpy (player->address, buffer2);
  strcpy (player->web_source, buffer3);
  snprintf (player->banner, sizeof (player->banner), "No Flag Set");
  snprintf (player->banner_source, sizeof (player->banner_source),
            "No Flag Set");

  generate_ship (player, 1, 0, !special);
  if (special)
    player->energy = 1500;
  else
    player->energy = 500;
  player->last_restart = turn;
  player->last_orders = turn;
  for (try = 0; try < 32; try++)
    {
      player->star = homeworlds[dice (25)];
      ok = TRUE;
      for (other = 0; other < MAX_PLAYER; other++)
        if (other != (player - players) &&
            players[other].star == player->star)
          ok = FALSE;
      if (ok)
        try = 32;
    }
  if (strcmp (player->address, "nobody@localhost") == 0)
    player->star = NOWHERE;
  if (player->star >= 0 && player->star < MAX_STAR)
    set_bit(player->stars, player->star);
  player->alliance =
    player < players + MAX_PLAYER ? PLAYER_ALLIANCE : SHOP_ALLIANCE;
  /* should depend on num_players but not set yet */
  player->health = 999;
  player->movement = player->star;
  if (sort == 2)
    player->password = passwd;
  else
    player->password = rand32();
  create_header (player);       /* existing one will be wrong now */
}

void
init_players ()
{
  int i;

  for (i = 0; i < MAX_PLAYER; i++)
    init_new_player (players + i, 1);
}


void
reset_aliens ()
{
  int i, j, best_good;
  struct PLAYER *target;
  struct ITEM *item;

  for (i = 0; i < MAX_ALIEN; i++)
    {
      aliens[i].alliance = i & 31;
      item = items + aliens[i].ship;
      while (item != items)
        {
          item->flags &= ~(ITEM_BROKEN);
          item = items + item->link;
        }
      aliens[i].countdown = 0;
      aliens[i].friends = 0;
      aliens[i].enemies = 0;
      sprintf (aliens[i].name, "%s %d", races[aliens[i].alliance].name, i);
      switch (races[aliens[i].alliance].style)
        {
        case archer:
        case sneaky:
        case balanced:
          sprintf (aliens[i].banner, "We come in peace for all %s-kind",
                   races[aliens[i].alliance].name);
          break;
        case trader:
          sprintf (aliens[i].banner, "A peaceful trader");
          break;
        case pirate:
          sprintf (aliens[i].banner, "Give me cargo pods");
          break;
        }
      target = pairing (aliens + i);
      if (!target)
        continue;
      if (target->alliance != PLAYER_ALLIANCE)
        continue;
      aliens[i].hide_hunt = -dice (2000);
      /* means hunt anywhere as dangerous as a dice place */
      aliens[i].strategy.ideal_range = find_best_range (aliens + i, target);
      aliens[i].strategy.firing_rate = 1;
      aliens[i].strategy.retreat =
        dice (10) + races[aliens[i].alliance].tech_level;

      best_good = BIG_NUMBER;
      item = items + target->ship;
      while (item != items)
        {
          if (item->sort == pod && item->reliability < best_good
              && item->reliability > SCRAP)
            {
              aliens[i].strategy.demand = item - items;
              best_good = item->reliability;
            }
          item = items + item->link;
        }
      if (best_good == BIG_NUMBER)
        for (j = 0; j < 4; j++)
          {
            aliens[i].strategy.demand = choose_target (items + target->ship);
            if (!(items[aliens[i].strategy.demand].flags & ITEM_BROKEN))
              j = 4;
          }
      items[aliens[i].strategy.demand].flags |= ITEM_DEMANDED;
      if ((target->enemies & (1 << aliens[i].alliance)))
        {
          aliens[i].strategy.dip_option = always_attack;
        }
      else if (any_good_range (aliens + i, target))
        switch (races[aliens[i].alliance].hostility)
          {
          case chaotic:
            if (dice (2))
              {
                aliens[i].strategy.dip_option = make_demands;
                break;
              }
          case hostile:
            aliens[i].strategy.dip_option = attack_if_defied;
            break;
          default:
            aliens[i].strategy.dip_option = flee;
            break;
          }
      else
        aliens[i].strategy.dip_option = flee;

      if (aliens[i].strategy.dip_option == flee)
        aliens[i].strategy.cbt_option = favour_fleeing;
      else
        switch (races[aliens[i].alliance].style)
          {
          case archer:
            aliens[i].strategy.cbt_option = favour_sensors;
            break;
          case sneaky:
            aliens[i].strategy.cbt_option = favour_cloaks;
            break;
          case pirate:
            aliens[i].strategy.cbt_option = favour_engines;
          case balanced:
          default:
            aliens[i].strategy.cbt_option = dice (5) + 1;
            break;
          }
    }
}

void
init_aliens ()
{
  int i, race;

  for (i = 0; i < MAX_ALIEN; i++)
    {
      aliens[i].ship = 0;   /* ie none */
    }
  reset_aliens ();
  for (race = 0; race < 32; race++)
    {
      races[race].plague = 10;
      races[race].wealth = 0;
    }
}

int
ship_size (struct ITEM *item)
{
  int result = 0;

  while (item != items)
    {
      result++;
      item = items + item->link;
    }
  return (result);
}

void
name_shops ()
{
  int shop;

  for (shop = 0; shop < MAX_SHOP; shop++)
    sprintf (shops[shop].name, "%s Shop-%d",
             star_names[shops[shop].star], shop);
}

void
reset_shops ()
{
  int shop, tech;
  struct ITEM *item;
  item_sort module;
  int shop_size = average_players/7;
  
  for (shop = 0; shop < MAX_SHOP; shop++)
    {
      item = items + shops[shop].ship;
      while (item != items)
        {
          if (item->sort < pod)
            {
              item->reliability += 7 - item->efficiency;
              if (item->reliability > 95)
                {
                  item->reliability = 95;
                }
            }
          if (item->sort == pod || item->reliability >= 95)
            {
              if (item->price > 25 * (1 << item->efficiency))
                {
                  item->price -= 1 << item->efficiency;
                }
              else
                {
                  int p = 40 - 40 / (7 - item->efficiency);
                  if (ship_size(items + shops[shop].ship) > rand_exp(shop_size)
                      && ! dice(1+p))
                    {
                      destroy_item (item - items);
                    }
                }
            }

          item = items + item->link;
        }
      tech = restock_tech;
      module = restock_item;

      if (restock_tech < 1 || restock_tech > 6)
        restock_tech = 1 + dice(6);
      if (module < warp_drive || module > pod)
        module = 1 + dice(pod);
      
      if (ship_size (items + shops[shop].ship) < rand_exp (shop_size))
        {
          shops[shop].ship = add_item (shops + shop,
                                       new_item (module, tech,
                                                 20 + dice (70),
                                                 dice (4), FALSE));
        }
    }
}


void
init_shops ()
{
  int shop;

  for (shop = 0; shop < MAX_SHOP; shop++)
    {
      //generate_ship (shops + shop, dice (6) + 1, dice (6), FALSE);
      generate_ship (shops + shop, weighted_tech (), dice (6), FALSE);
      shops[shop].star = shop < 32 ? homeworlds[shop] : dice (MAX_HAB_STAR);
      shops[shop].alliance = SHOP_ALLIANCE;
    }
  reset_shops ();
}

int
weighted_tech ()
{
  int rand_int;

  rand_int = dice (20) + 1;
  switch (rand_int)
    {
    case 1:
    case 2:
    case 3:
    case 4:
      return (1);
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
      return (2);
    case 11:
    case 12:
    case 13:
    case 14:
      return (3);
    case 15:
    case 16:
    case 17:
      return (4);
    case 18:
    case 19:
      return (5);
    case 20:
      return (6);
    default:
      return (1);
    }
}

void
reset_npcs ()
{
  reset_aliens ();
  reset_adventures ();
}

void
reset_stars ()
{

  int star;

  for (star = MAX_HAB_STAR; star < MAX_STAR; star++)
    {
      star_names[star] = malloc (10);
      sprintf (star_names[star], "Star #%d", star - MAX_HAB_STAR);
    }
}

void
init_game ()
{
  generate_board ();
  init_prices ();
  init_players ();
  init_aliens ();               /* must be after players as enemies are modified */
  init_shops ();
  name_shops ();
  init_adventures ();
}

void
upgrade (struct PLAYER *player)
{
  struct ITEM *item = items + player->ship;

  while (item != items)
    {
      if (item->sort >= shield)
        item->efficiency += 2;
      else
        item->efficiency++;
      item = items + item->link;
    }
}

int
total_item (item_sort sort, struct ITEM *item)
{
  int result = 0;

  do
    {
      if (item->sort == sort)
        result += item->efficiency;
      item = items + item->link;
    }
  while (item != items);
  return (result);
}

int
total_working_item (item_sort sort, struct ITEM *item)
{
  int result = 0;

  do
    {
      if (item->sort == sort && !(item->flags & ITEM_BROKEN))
        result += item->efficiency;
      item = items + item->link;
    }
  while (item != items);
  return (result);
}


void
consolidate_artifacts ()
{
  int player, flag, i;
  struct ITEM *item;

  for (player = 0; player < MAX_SHIP; player++)
    {
      players[player].artifacts = players[player].blessings;

      for (i = 0; i < 4; i++)
        {
          if (prophets[i] == player)
            players[player].artifacts |= 0x30000 << (i * 2);
          if (ministers[i] == player)
            players[player].artifacts |= 0x1000000  << i;
        }

      if (star_has_loc (players[player].star, terminal) != NO_LOCATION)
        {
          for (flag = 0; flag < 16; flag++)
            if (players[player].passwd_flags & (1 << flag))
              players[player].artifacts |= password_bonus[flag].blessing;
        }
      item = items + players[player].ship;
      while (item != items)
        {
          if (item->sort == artifact && !(item->flags & ITEM_BROKEN))
            players[player].artifacts |= (item->magic & 0xffffff);
          item = items + item->link;
        }
    }
}

void
debug (FILE * fd, struct PLAYER *player)
{
  struct ITEM *item;

  fprintf (fd, "%s's bits are:\n", player->name);
  item = items + player->ship;
  while (item != items)
    {
      fprintf (fd, "%d ", item - items);
      item = items + item->link;
    }
  fprintf (fd, "<BR>");
}

int
protected_items (short item)
{
  int result = 0, it = item;

  while (it)
    {
      if (items[it].flags & ITEM_PROTECTED)
        result++;
      it = items[it].link;
    }
  if (result == 0)
    {
      it = item;
      while (it)
        {
          items[it].flags |= ITEM_PROTECTED;
          it = items[it].link;
          result++;
        }
    }
  return (result ? result : 1);
  /* don't return 0 because it'll break, best notice dead ship */
}

int
is_wraith (struct PLAYER *player)
{
  return (player->rings_held & 0xf);
}

int
is_player (struct PLAYER *player)
{
  return (player < aliens);
}


void
new_ranking (char *p)
{
  int plus = TRUE;
  switch (*p)
    {
    case '-':
      plus = FALSE;
      break;
    case '\0':
      big_ranking (times, atoi (p), "User Ranking");
      break;
    }
}

void
press (int player, const char *text, const char *author)
{
  int rumour;
  char buf1[1024], buf2[1024];
  int s = 0, t = 0;

  if (want_verbose)
    printf ("Player %d writing as %s submitted the following press:\n%s\n",
      player, author, text);

  memset(buf1, '\0', sizeof(buf1));
  while (text[s])
  {
    switch (text[s])
    {
      case '\"':
        sprintf (buf1, "%s%s", buf1, "&quot;");
        t += 6;
        s++;
        break;
      default:
        buf1[t++] = text[s++];
        break;
    }
  }

  do
    rumour = dice (MAX_RUMOUR);
  while (rumours[rumour]);

  if (player)
    sprintf (buf2, "<a href=\"http://%s/cgi-bin/mail.crm?id=%d\">%s</a>",
             server, player, author);
  else
    strcpy (buf2, author);
 
  rumours[rumour] = malloc (strlen (buf1) + 300 + strlen (buf2));

  if (buf1[0] == '!')
    sprintf (rumours[rumour], "%s", buf1);
  else
    sprintf (rumours[rumour],
             "<HR><iframe sandbox seamless srcdoc=\"<div class=\"press\"><div class=\"rumor\">%s</div>\n<div class=\"author\">%s</div></div>\"></iframe>\n",
             buf1, buf2); 
}

/*
void
press (int player, const char *text, const char *author)
{
  int state = 0, rumour;
  const char *p = text;
  char buffer[1024];

  if (want_verbose)
    printf ("Player %d writing as %s submitted the following press:\n%s\n", 
      player, author, text);
  // check for embedded <HR>
  while (*p)
    {
      switch (*p++)
        {
        case ' ':
        case '\n':
        case '\t':
          break;
        case '<':
          if (state == 0)
            state = 1;
          else
            state = 0;
          break;
        case 'h':
        case 'H':
          if (state == 1)
            state = 2;
          else
            state = 0;
          break;
        case 'r':
        case 'R':
          if (state == 2)
            state = 3;
          else
            state = 0;
          break;
        case '>':
          if (state == 3)
            return;
          state = 0;
        default:
          state = 0;
        }
    }
  do
    rumour = dice (MAX_RUMOUR);
  while (rumours[rumour]);
  if (player)
    sprintf (buffer, "<a href=\"http://%s/cgi-bin/mail.crm?id=%d\">%s</a>",
             server, player, author);
  else
    strcpy (buffer, author);
  rumours[rumour] = malloc (strlen (text) + 160 + strlen (buffer));
  if (text[0] == '!')
    sprintf (rumours[rumour], "%s", text);
  else
    sprintf (rumours[rumour],
             "<HR><table><tr><th>%s</th></tr>\n<tr><td>%s</td></tr></table>\n",
             buffer, text);
}
*/

int
decode_starname (char *name, int current)
{
  int len, result;
  char buffer[256];

  standardise_name (name, buffer);
  len = strlen (buffer);
  for (result = 0; result < MAX_STAR; result++)
    if (strncmp (buffer, star_names[result], len) == 0)
      return (result);
  return (current);
}

char *good_servers[10] = {
  "www.angelfire.com",
  "B",
  "C",
  "D",
  "E",
  "F",
  "www.geocities.com",
  "H"
};

void
remove_html (char *buffer)
{
  char *q;
  char local[128];

  while ((q = strchr (buffer, '<')) != (char *) 0)
    *q = '(';
  while ((q = strchr (buffer, '>')) != (char *) 0)
    *q = ')';
  if (buffer[0] == '#')
    {
      while ((q = strchr (buffer, '?')) != (char *) 0)
        *q = ' ';

      buffer[40] = '\0';
      sprintf (local, "<img src=\"http://%s/images/%s\">",
               server, buffer + 1);
      strcpy (buffer, local);
    }
}

int
shop_owns_item (int item)
{
  int shop;

  for (shop = 0; shop < MAX_SHOP; shop++)
    if (owner (shops + shop, item))
      return (TRUE);
  return (FALSE);
}

void
disable (FILE * fd, struct PLAYER *player, int item)
{
  if (! items[item].sort < pod)
    {
      fprintf (fd, "<P>Can't shut down %s.\n",
               item_string (items + item));
    }
  else if (! owner (player, item))
    {
      fprintf (fd, "<P>Do you even own %s?\n",
               item_string (items + item));
    }
  else
    {
      fprintf (fd, "<P>Shut down %s\n",
               item_string (items + item));
      (items + item)->flags |= ITEM_BROKEN;
    }
}

void
shop_at (FILE * fd, struct PLAYER *player, int item)
{
  if (owner (player, item))     /* selling */
    {
      fprintf (fd, "<P>Sold %s for $%d\n",
               item_string (items + item), sale_price (items + item));
      player->energy += sale_price (items + item);
      destroy_item (item);
    }
  else                          /* buying */
    {
      if (player->energy < items[item].price)
        {
          fprintf (fd, "<P><EM>Can't afford shopping</EM>\n");
          return;
        }
      if (!shop_owns_item (item))
        {
          fprintf (fd, "<P><EM>%s no longer available</EM>\n",
                   item_string (items + item));
          return;
        }
      player->energy -= items[item].price;
      fprintf (fd, "<P>Bought %s for $%d\n",
               item_string (items + item), items[item].price);
      player->ship = transfer_item (item, player);
    }
}

void
collect (FILE * fd, struct PLAYER *player, int loc, char qualifier)
{
  int parameter;
  item_sort sort;
  int amount, count, found, damage, target, price, value;
  skill_sort skill;
  int race;

  if (player->star >= MAX_STAR)
    {
      printf ("Attempt to collect from holiday planet: %d\n",
              player->star);
      return;
    }

  if (loc >= BIG_NUMBER)
    {

      loc -= BIG_NUMBER;
      sort = constituency;
      if (locations[loc].sort == homeworld)
        race = locations[loc].parameter;
      else
        race = RACE_NUMBER (locations[loc].parameter);
      skill = races[race].religion;
    }
  else
    {
      parameter = locations[loc].parameter;
      sort = locations[loc].sort;
    }

  switch (sort)
    {
    case constituency:
      if (locations[loc].used)
        fprintf (fd,
                 "<P><EM>Someone more impressive already campaigned for votes at %s %s</EM>\n",
                 races[race].name, loc_string (loc));
      else
        {
          int influence =
            factor (skill * 2, player) + factor (skill * 2 + 1, player);
          int p = player - players;

          fprintf (fd,
                   "<P>%s officer campaigned at %s %s with %d influence\n",
                   skill_names[skill], races[race].name, loc_string (loc),
                   influence);
          if (locations[loc].voter == 0 || locations[loc].voter == p)
            {
              locations[loc].influence += influence;
              locations[loc].voter = p;
            }
          else
            {
              locations[loc].influence -= influence;
              if (locations[loc].influence == 0)
                locations[loc].voter = 0;
              if (locations[loc].influence < 0)
                {
                  locations[loc].influence = -locations[loc].influence;
                  locations[loc].voter = p;
                  fprintf (fd, "<BR>and took control of the location\n");
                }
            }
          locations[loc].used = TRUE;
        }
      break;
    case academy:
      if (player->
          skills[parameter] & skill_bit (academy_skill, qualifier - 'A'))
        {
          fprintf (fd,
                   "<P><EM>%s officer has already taken that academy course</EM>\n",
                   skill_names[parameter]);
          break;
        }
      if (player->energy < (qualifier - 'A') * (qualifier - 'A') * 100)
        {
          fprintf (fd, "<P><EM>Can't afford academy fee</EM>\n");
          break;
        }
      if (skill_level (player->skills[parameter]) < (qualifier - 'A') * 4 - 4)
        {
          fprintf (fd, "<P><EM>Need skill %d for that academy course</EM>\n",
                   (qualifier - 'A') * 4 - 4);
          break;
        }
      player->energy -= (qualifier - 'A') * (qualifier - 'A') * 100;
      player->skills[parameter] |= skill_bit (academy_skill, qualifier - 'A');
      fprintf (fd, "<P>Studied at %s academy\n", skill_names[parameter]);
      break;
    case arsenal:
      fprintf (fd, "<P>Bought %d Photon Torpedoes\n", qualifier - 'A');
      player->energy -= 10 * (qualifier - 'A');
      player->torps += (qualifier - 'A');
      break;
    case badland:
    case gas_giant:
      recruit_rogues (fd, player, loc);
      break;
    case belt:
      amount = factor (impulse_drive, player);
      if (amount > stars[player->star].ore)
        amount = stars[player->star].ore;
      stars[player->star].ore -= amount;
      amount = fuzz(5*amount);
      player->energy += amount;
      fprintf (fd, "<P>Collected ore worth $%d\n", amount);
      break;
    case colony:
      if ((price = sell (fd, player, parameter)))
        fprintf (fd, "<P>Sold %s to %s Colony for $%d\n",
                 goods[GOOD_NUMBER (parameter)].name,
                 races[RACE_NUMBER (parameter)].name, price);
      break;
    case comet:
      if (locations[loc].used)
        {
          fprintf (fd,
                   "<P><EM>Chocolate already collected from these comets this turn</EM>\n");
          break;
        }
      if (load_pod (items + player->ship, CHOCOLATE, 1))
        {
          fprintf (fd, "<P>Collected chocolate comets\n");
          locations[loc].used = TRUE;
        }
      else
        fprintf (fd, "<P><EM>No room to load chocolate</EM>\n");
      break;
    case corona:
      if (!loc_accessible (player, loc))
        {
          fprintf (fd, "<P><EM>Can't get close enough to skim star</EM>\n");
          break;
        }
      amount = fuzz(5 * locations[loc].risk);
      fprintf (fd, "<P>Gained $%d skimming star", amount);
      player->energy += amount;
      if (dice (100) < locations[loc].risk)
        {
          target = choose_target (items + player->ship);
          if (items[target].sort >= pod)
            break;
          items[target].flags |= ITEM_BROKEN;
          items[target].reliability -= dice (20);
          fprintf (fd, "<BR><EM>Unexpected Flare damages %s!</EM>\n",
                   item_string (items + target));
          if (items[target].reliability > 200)
            {
              fprintf (fd, "<BR><EM>Damage destroys %s!</EM>\n",
                       item_string (items + target));
              destroy_item (target);
            }
        }
      break;
    case deep_space:
    case near_space:
      if (player->star == popcorn.star)
        harvest_popcorn (fd, player);
      else
        fprintf (fd, "<P><EM>The Popcorn source is not here</EM>\n");
      break;
    case factory:
      if (qualifier < 'a')
        amount = qualifier - 'A';
      else
        amount = 'a' - qualifier - 1;
      while (amount < 0)        /* means sell scrap really */
        {
          if (unload_pod (fd, player, SCRAP))
            {
              player->energy += 25;
              fprintf (fd, "<P>Sold scrap to Factory for $25\n");
              amount++;
            }
          else
            {
              fprintf (fd, "<P><EM>Failed to sell scrap</EM>\n");
              amount = 0;
            }
        }
      while (amount &&
             player->energy >= goods[GOOD_NUMBER (parameter)].basic_value &&
             load_pod (items + player->ship, GOOD_NUMBER (parameter), 1))
        {
          amount--;
          fprintf (fd, "<P>Bought 1 %s from factory for $%d\n",
                   goods[GOOD_NUMBER (parameter)].name,
                   goods[GOOD_NUMBER (parameter)].basic_value);
          player->energy -= goods[GOOD_NUMBER (parameter)].basic_value;
        }
      if (amount > 0)
        fprintf (fd, "<P><EM>Failed to buy from factory</EM>\n");
      break;
    case homeworld:
      amount =
        (races[parameter].wealth * (100 - races[parameter].plague)) / 100;
      fprintf (fd, "<P>Collected $%d from %s homeworld\n", amount,
               races[parameter].name);
      player->energy += amount;
      races[parameter].wealth = 0;
      player->enemies |= 1 << parameter;
      player->favour[weaponry] += races[parameter].tech_level;
      break;
    case hall:
      skill = qualifier - 'A';
      if (skill_level (player->skills[skill]) <= player->crew[skill])
        {
          fprintf (fd, "<P><EM>Can't hire any more %s crew</EM>\n",
                   skill_names[skill]);
          break;
        }
      if (ministers[TRIBUNE] == player - players)
        {
          int s = skill_level(player->skills[skill]);
          int number = 1 + ((s > 2) ? dice(s - 1) : 1);
          number = min (number, s - player->crew[skill]);
          if (player->artifacts & (0x1000000 << skill))
            {
              add_crew (player, skill, 1, s);
              if (number - 1)
                add_crew (player, skill, number - 1, 0);
              fprintf (fd, "<P>Hired %d (1 skilled) %s crew wanting to work for the Tribune\n",
                       number, skill_names[skill]);
            }
          else
            {
              add_crew (player, skill, number, 0);
              fprintf (fd, "<P>Hired %d %s crew wanting to work for the Tribune\n",
                       number, skill_names[skill]);
            }
        }
      else
        {
          if (player->artifacts & (0x1000000 << skill))
            add_crew (player, skill, 1, skill_level (player->skills[skill]));
          else
            add_crew (player, skill, 1, 0);
          fprintf (fd, "<P>Hired %s crew\n", skill_names[skill]);
        }
      break;
    case minefield:
      if (!loc_accessible (player, loc))
        {
          fprintf (fd,
                   "<P><EM>Cloaks not good enough to enter minefield safely</EM>\n");
          break;
        }
      count = player->crew[weaponry];
      fprintf (fd, "<P>Tried to collect %d photon torpedoes, ", 5*count);
      found = damage = 0;
      while (count--)
        {
          if (dice (100) < locations[loc].risk)
            found += dice(5);
          if (locations[loc].risk >
              dice (100) + factor (life_support, player))
            damage++;
        }
      fprintf (fd, "and found %d. Health damage is %d%%", found, damage);
      player->torps += found;
      player->health -= damage;
      break;
    case ocean:
      value = rand_exp(50) + 1;
      if (value >= 1000)
        value = 999;
      if (factor (sick_bay, player) > value)
        {
          value += dice (32) * 1000;
          fprintf (fd, "<P>Discovered new %s medicine of value $%d\n",
                   races[value / 1000].name, (value % 1000) * 50);
          if (player->medicine)
            fprintf (fd, "<BR>Had to discard old medicine to make room\n");
          player->medicine = value;
        }
      else
        fprintf (fd, "<P><EM>No new medicine found</EM>\n");
      break;
    case prison:
      if (player->prisoner)
        {
          amount = fuzz(500 * (player->prisoner & 7) * (player->prisoner & 7));
          fprintf (fd, "<P>Collected $%d reward for %s\n",
                   amount, criminal_string (player->prisoner));
          relocate_criminal (player->prisoner);
          player->prisoner = NO_CRIMINAL;
          player->energy += amount;
        }
      else
        fprintf (fd,
                 "<P><EM>You have no prisoner to cash in for reward</EM>\n");

      break;
    case school:
      if (player->energy < (qualifier - 'A') * 50)
        {
          fprintf (fd, "<P><EM>Can't afford %s schooling</EM>\n",
                   skill_names[parameter]);
          break;
        }
      if (player->
          skills[parameter] & skill_bit (school_skill, qualifier - 'A'))
        {
          fprintf (fd, "<P><EM>Already been to %s school</EM>\n",
                   skill_names[parameter]);
          break;
        }
      player->energy -= (qualifier - 'A') * 50;
      player->skills[parameter] |= skill_bit (school_skill, qualifier - 'A');
      fprintf (fd, "<P>Studied at %s school\n", skill_names[parameter]);
      break;
    case terminal:
      fprintf (fd, "<P>Gained access to Starnet Terminal #%d\n", parameter);
      player->experience[science] |= 1 << parameter;
      break;
    default:
      printf ("Funny collect from S%02d, %d\n", player->star, loc);
      break;
    }
}

int
any_gates (struct PLAYER *player, int from, int to)
{
  int loc, key;

  for (loc = 0; loc < MAX_LOCATION; loc++)
    if (locations[loc].star == from &&
        locations[loc].sort == stargate && locations[loc].parameter == to)
      {
        key = (from ^ to) & 7;
        if (player->artifacts & (1 << key))
          return (TRUE);
      }
  return (FALSE);
}


void
explore (FILE * fd, struct PLAYER *player, int loc)
{
  int ad, found = FALSE;
  int criminal = locations[loc].criminal;
  int ring = locations[loc].ring;

  for (ad = 0; ad < MAX_ADVENTURE; ad++)
    {
      if (adventures[ad].loc == loc)
        {
          fprintf (fd, "<P>Found %s exploring %s (%d)!<BR>%s\n",
                   ad_types[ADVENTURE_TYPE (ad)].ad_name,
                   loc_string (loc), loc,
                   ad_types[ADVENTURE_TYPE (ad)].ad_desc);
          fprintf (fd, " (Needs %s skill level %d)\n",
                   skill_names[ADVENTURE_SKILL (ad)], ADVENTURE_LEVEL (ad));
          set_ad (player, ad);
          found++;
        }
    }
  if (!found)
    fprintf (fd, "<P><EM>No new adventures found exploring %s (%d)</EM>\n",
             loc_string (loc), loc);
  if ((criminal & 7) == 1)
    {
      set_crim (player, criminal);
      fprintf (fd, "<P>Detected %s at %s\n",
               criminal_string (criminal), loc_string (loc));
    }
  else
    fprintf (fd, "<P><EM>No new criminals detected at %s (%d)</EM>\n",
             loc_string (loc), loc);
  if (ring)
    {
      printf ("%s saw a ring\n", player->name);
      player->rings_seen |= ring;
      fprintf (fd, "<P>Discovered %s at %s\n",
               ring_string (ring), loc_string (loc));
    }
}

void
cure (FILE * fd, struct PLAYER *player)
{
  int amount = factor (sick_bay, player);
  int race = who_home (player->star);

  if (amount > races[race].plague)
    amount = races[race].plague;
  races[race].plague -= amount;
  player->experience[medical] |= 1 << race;
  fprintf (fd, "<P>Reduced plague by %d%%\n", amount);
}


void
train (FILE * fd, struct PLAYER *player, skill_sort skill)
{
  if (player->crew[skill] == 0)
    {
      fprintf (fd, "<P><EM>No %s crew to train</EM>\n", skill_names[skill]);
      return;
    }
  player->pools[skill] += skill_level (player->skills[skill]);
  if ((player->pools[skill] / player->crew[skill]) >
      skill_level (player->skills[skill]))
    player->pools[skill] = player->crew[skill] *
      skill_level (player->skills[skill]);
  fprintf (fd, "<P>%s crew trained up to average of %d\n",
           skill_names[skill], player->pools[skill] / player->crew[skill]);
}

void
open_results (FILE ** fd, struct PLAYER *player)
{
  char buffer[128];

  if (*fd)
    fclose (*fd);
  sprintf (buffer, "%s/results/%d/%s%d.a", webroot,
           game, player->name, turn);
  if (player->results)
    *fd = fopen (buffer, "a");
  else
    *fd = fopen (buffer, "w");
  if (!*fd)
    {
      printf ("Can't create results %s\n", buffer);
      exit (1);
    }
  player->results = TRUE;
}


int
give_favour (FILE * fd, skill_sort skill, struct PLAYER *player,
             struct PLAYER *target)
{
  if (player->star == HOLIDAY || player->star >= MAX_STAR)
    return (FALSE);             /* no ring giving on holiday */
  if (player->favour[skill] < 10)
    {
      printf ("%s losing ring\n", player->name);
      fprintf (fd, "<P><EM>Insufficient favour to keep %s ring!</EM>\n",
               skill_names[skill]);
      player->rings_held &= ~(0x10 << skill);
      init_ring (skill + 4);
      return (FALSE);
    }
  fprintf (fd, "<P>You give some %s favour to %s\n",
           skill_names[skill], name_string (target->name));
  player->favour[skill] -= 10;
  target->favour[skill] += 10;
  return (TRUE);
}

int pw_stats[4] = { 0, 0, 0, 0 };

void
check_passwords (FILE * fd, struct PLAYER *player)
{
  int loc = star_has_loc (player->star, terminal);
  int fragment = dice (4), mask = 0xff, flag;

  if (loc != NO_LOCATION && (player->experience[science] & (1 << loc)))
    {
      pw_stats[fragment]++;
      mask <<= (fragment * 8);
      if (password_key & (1 << fragment))
        fragment = (password_true & mask) >> (fragment * 8);
      else
        fragment = (password_false & mask) >> (fragment * 8);
      fprintf (fd, "<P>Starnet Terminal reveals password fragment %s\n",
               byte_name (fragment));
      if (player->passwd_flags)
        fprintf (fd, "<BR>Starnet hacking provides benefits next turn\n");
    }
  if (player->passwd_flags)
    fprintf (fd, "<BR>Starnet hacking benefits are:\n");
  for (flag = 0; flag < 16; flag++)
    {
      if (player->passwd_flags & (1 << flag))
        fprintf (fd, "<LI>%s\n", password_bonus[flag].name);
    }
}

void
fix_criminals ()
{
  int loc;

  for (loc = 0; loc < MAX_LOCATION; loc++)
    {
      if (locations[loc].criminal
          && !(location_types[locations[loc].sort].flags & LOC_CRIMINAL))
        {
          printf ("%s in %s\n", criminal_string (locations[loc].criminal),
                  loc_string (loc));
          relocate_criminal (locations[loc].criminal);
          locations[loc].criminal = NO_CRIMINAL;
        }
    }
}

void
check_allies (FILE * fd, int player)
{
  int p;

  for (p = 0; p < MAX_PLAYER; p++)
    if (p != player && players[p].companion == player)
      {
        fprintf (fd, "<P>%s has named you as their ally\n",
                 name_string (players[p].name));
        if (players[p].gift)
          fprintf (fd, " and gave you $%d this turn\n", players[p].gift);
      }
}

void
end_turn ()
{
  struct PLAYER *p = players + 1;
  int race, pay, i;
  FILE *fd;
  char buffer[80];
  skill_sort skill;

  fix_criminals ();
  /* make new passwords for next turn */
  password_true = rand32();
  password_false = rand32();
  password_false = ~password_true;
  password_key = dice (16);

  reset_aliens ();
  for (i = 1; i < MAX_PLAYER; i++)
    for (skill = engineering; skill <= weaponry; skill++)
      if (players[i].chosen & (1 << skill))
        chosen[skill]++;
  influence_decay ();
  consolidate_votes ();
  while (p < players + num_players)
    {
      sprintf (buffer, "%s/results/%d/%s%d.c",
               webroot, game, p->name, turn);
      fd = fopen (buffer, "w");
      if (!fd)
        {
          printf ("Can't open end-turn file\n");
          exit (1);
        }
      check_reliability (fd, p);
      check_health (fd, p);
      check_favour (fd, p);
      update_evil (fd, p);
      check_evil (fd, p);
      check_allies (fd, p - players);
      check_passwords (fd, p);
      check_votes (fd, p);

        
      if (p->star != HOLIDAY && p->star < MAX_STAR)
        {
          fprintf (fd, "<h3>Finances</h3>\n");
          if (p->popcorn > 0)
            {
              fprintf (fd, "<P>You have %d popcorn\n", p->popcorn);
            }
          p->energy += 50;
          fprintf (fd, "<P>Ship's power plant generates $50\n");
          
          pay = total_collection (items + p->ship);
          p->energy -= pay;
          fprintf (fd, "<P>Modules consume energy of $%d\n", pay);

          if (p->energy < 0)
            p->energy = 0;

          pay = total_pay (p);
          p->energy -= pay;
          if (p - players == ministers[PRESIDENT])
            {
              fprintf (fd, "<P>After government covers crew costs, total pay for crew and mercenaries is $%d\n", pay);
              if (p->energy < 0)
                p->energy = 0;
            }
          else
            {
              fprintf (fd, "<P>Total pay for crew and mercenaries is $%d\n", pay);
            }
          while (p->energy < 0)
            {
              for (skill = engineering; skill < weaponry; skill++)
                if (dice (p->crew[skill]))
                  {
                    kill_crew (p, skill);
                    fprintf (fd,
                             /* strings concatenated */
                             "<BR><EM>A %s crew-person deserted "
                             "for lack of pay</EM>\n",
                             skill_names[skill]);
                  }
              p->energy++;
            }
        }
      else
        {
          if (mothballed (p - players))
            {
              /* note use of string concatenation */
              fprintf (fd, "You are at the Mothball Planet, "
                       "and no longer hold any scarce resources, "
                       "such as rings, ministries, prophethoods, etc.");
              mothball_player (p);
            }
        }
      p++;
      fclose (fd);
    }
  for (race = 0; race < 32; race++)
    {
      races[race].wealth +=
        25 * races[race].tech_level * races[race].tech_level;
      races[race].wealth *= (100 - races[race].plague);
      races[race].wealth /= 100;
      races[race].plague += rand_exp (average_players/16);
      if (races[race].plague < 1)
        races[race].plague = 1;
      if (races[race].plague > 99)
        races[race].plague = 99;
    }
}

void
scrap (FILE * fd, struct PLAYER *player, int code)
{
  struct ITEM *item;

  if (code < 0)                 /* scrap cargo */
    {
      item = items - code;
      if (item->reliability < BASE_UNIT)        /* scrap goods */
        {
          fprintf (fd, "<P>Scrapped %d units of %s\n",
                   item->collection, goods[item->reliability].name);
          item->reliability = SCRAP;
        }
      else                      /* demob unit */
        {
          fprintf (fd, "<P>Demobbed %s\n",
                   units[item->reliability - BASE_UNIT].name);
          units[item->reliability - BASE_UNIT].pay = 0;
          item->reliability = 0;
          item->collection = 0;
          player->flags |= DISGRACED;
        }
    }
  else                          /* scrap module */
    {
      item = items + code;
      if (load_pod (items + player->ship, SCRAP, 1))
        {
          fprintf (fd, "<P>Scrapped %s\n", item_string (item));
          destroy_item (code);
        }
      else
        fprintf (fd, "<P><EM>Can't scrap %s</EM>\n", item_string (item));
    }
}

void
show_experience (FILE * fd, struct PLAYER *player)
{
  int loc, ad, parameter, crim;

  fprintf (fd, "<H2>");
  print_rules_link(fd, "Adventures", "Adventures");
  fprintf (fd, " Known at:</H2>\n");
  for (ad = 0; ad < MAX_ADVENTURE; ad++)
    if (get_ad (player, ad))
      {
        parameter = adventures[ad].parameter;
        fprintf (fd, "<LI>%s-%d (%s) in %s-%d at %s<BR>\n",
                 skill_names[ADVENTURE_SKILL (parameter)],
                 ADVENTURE_LEVEL (parameter),
                 ad_types[ADVENTURE_TYPE (parameter)].ad_name,
                 loc_string (adventures[ad].loc),
                 adventures[ad].loc,
                 star_names[adventures[ad].star]);
      }

  fprintf (fd, "<H2>");
  print_rules_link(fd, "Terminals", "Terminals");
  fprintf (fd, " Accessed at:</H2>\n");
  for (loc = 0; loc < MAX_LOCATION; loc++)
    if (locations[loc].sort == terminal
        && player->experience[science] & (1 << locations[loc].parameter))
      fprintf (fd, "%s \n", star_names[locations[loc].star]);
  if (player->experience[science] == 0)
    fprintf (fd, "None\n");

  fprintf (fd, "<H2>");
  print_rules_link(fd, "Cure", "Plagues");
  fprintf (fd, " Cured at:</H2>\n");
  for (loc = 0; loc < MAX_LOCATION; loc++)
    if (locations[loc].sort == homeworld
        && player->experience[medical] & (1 << locations[loc].parameter))
      fprintf (fd, "%s \n", star_names[locations[loc].star]);
  if (player->experience[medical] == 0)
    fprintf (fd, "None\n");

  fprintf (fd, "<H2>");
  print_rules_link(fd, "Criminals", "Criminals");
  fprintf (fd, " Known:</H2>\n");
  for (crim = 0; crim < MAX_CRIMINAL; crim++)
    if (get_crim (player, crim))
      {
        loc = find_criminal (crim);
        if (loc == -1)
          fprintf (fd, "<BR>%s in captivity\n", criminal_string (crim));
        else
          fprintf (fd, "<BR>%s in %s at %s\n",
                   criminal_string (crim),
                   loc_string (loc), star_names[locations[loc].star]);
      }
}

void
pw_train (FILE * fd, struct PLAYER *player, skill_sort sort)
{
  if (player->crew[sort] == 0)
    return;
  fprintf (fd, "<BR>%s crew access starnet terminal for extra training\n",
           skill_names[sort]);
  player->pools[sort] += player->crew[sort];
  if ((player->pools[sort] / player->crew[sort]) >
      skill_level (player->skills[sort]))
    player->pools[sort] = player->crew[sort] *
      skill_level (player->skills[sort]);
}

void
do_password_powers (FILE * fd, struct PLAYER *player)
{
  if (star_has_loc (player->star, terminal) == NO_LOCATION)
    return;
  if (player->passwd_flags == 0)
    return;
  if (player->passwd_flags & PW_ENG_TRAIN)
    pw_train (fd, player, engineering);
  if (player->passwd_flags & PW_SCI_TRAIN)
    pw_train (fd, player, science);
  if (player->passwd_flags & PW_MED_TRAIN)
    pw_train (fd, player, medical);
  if (player->passwd_flags & PW_WEA_TRAIN)
    pw_train (fd, player, weaponry);
}

void
mudd (FILE * fd, struct PLAYER *player)
{
  short item;

  printf ("%s trying emergency warp drive\n", player->name);
  if (player->energy < 500)
    {
      fprintf (fd, "<P><EM>Can't afford emergency warp drive</EM>\n");
    }
  else
    {
      item = new_item (warp_drive, 1, 95, 1, TRUE);
      fprintf (fd, "<P>Bought a %s for $500\n", item_string (items + item));
      player->ship = add_item (player, item);
      player->energy -= 500;
    }
}

void
long_range_scan (FILE * fd, struct PLAYER *player)
{
  int seen_any = FALSE;
  int s, r;
  
  int f = factor (sensor, player);
  for (s = 0 ; s < MAX_STAR ; s++)
    {
      int r = effective_range (distance(player->star, s), f);
      if (!  get_bit (player->stars, s)
          && dice (100) < 200 - r
          && ! stars[s].hidden)
        {
          set_bit (player->stars, s);
          fprintf (fd, "<P>Star detected at %d, %d\n", stars[s].x, stars[s].y);
          fprintf (fd, "<BR>It matches the spectral catalog for %s",
                   star_names[s]);
          fprintf (fd, " and seems to have %s terrain\n",
                   terrain_names[stars[s].terrain]);
          seen_any = TRUE;
        }
    }
    
  s = popcorn.star;
  r = effective_range (distance(player->star, s), f);
  if (get_bit (player->stars, s)
      && dice(100) < 200 - r
      && ! stars[s].hidden)
    {
      fprintf (fd, "<P>Popcorn detected at %s\n", star_names[popcorn.star]);
      seen_any = TRUE;
    }

  s = OLYMPUS;
  r = effective_range (distance(player->star, s), f);
  if (dice(100) < 200 - r
      && player->chosen
      && ! stars[s].hidden)
    {
      fprintf (fd, "<P>Star detected at %d, %d\n",
               stars[OLYMPUS].x, stars[OLYMPUS].y);
      fprintf (fd, "<BR>It appears to be enclosed by a Dyson Sphere,\n");
      fprintf (fd, "like Olympus, legendary home of the Great Old Ones\n");
      player->chosen |= OLYMPUS_SEEN;
      seen_any = TRUE;
    }

  if (!seen_any)
    fprintf (fd, "<P><EM>Nothing unusual in scanning range</EM>\n");
}

void
change_companion (FILE * fd, struct PLAYER *player, int value)
{
  if (value == BIG_NUMBER)
    {
      fprintf (fd, "<P>Alliance with %s renounced\n",
               name_string (players[player->companion].name));
      player->companion = 0;
    }
  else if (value > BIG_NUMBER)
    {
      value -= BIG_NUMBER;
      if (value > player->energy)
        value = player->energy;
      fprintf (fd, "<P>$%d given to %s\n",
               value, name_string (players[player->companion].name));
      player->energy -= value;
      players[player->companion].energy += value;
      player->gift = value;
    }
  else
    {
      player->companion = value;
      fprintf (fd, "<P>Alliance offered to %s\n",
               name_string (players[player->companion].name));
    }
}

int
ground_strength (struct PLAYER *player, location_sort sort)
{
  struct ITEM *item = items + player->ship;
  int result = 0;

  while (item != items)
    {
      if (item->sort == pod && item->reliability >= BASE_UNIT)
        {
          int u = item->reliability - BASE_UNIT;

          result += local_strength (units[u].sort, sort, 1);
        }
      item = items + item->link;
    }
  return (result);
}

void
unload_medicine (FILE * fd, struct PLAYER *player)
{
  int race, fee;

  race = player->medicine / 1000;
  fee = (player->medicine % 1000) * 50;
  if (homeworlds[race] == player->star)
    {
      fprintf (fd, "<P>Medicine sold to %s homeworld for $%d\n",
               races[race].name, fee);
      player->medicine = 0;
      player->energy += fee;
    }
}

void
move_wraiths ()
{
  int num_ships[MAX_STAR + MAX_PLAYER], s, top = 0;

  memset (num_ships, 0, sizeof (num_ships));
  for (s = 0; s < MAX_PLAYER + MAX_ALIEN; s++)
    if (ships[s].star >= 0)
      num_ships[ships[s].star]++;
  for (s = 0; s < MAX_STAR; s++)
    if (num_ships[top] < num_ships[s])
      top = s;
  printf ("Most popular star is %s\n", star_names[top]);
  for (s = 0; s < MAX_PLAYER; s++)
    if (is_wraith (players + s) && players[s].star != HOLIDAY
        && players[s].star < MAX_STAR)
      players[s].star = top;
}

void
hide_systems ()
{
  int p;

  for (p = 1; p < MAX_PLAYER; p++)
    if (players[p].magic_flags & FLAG_HIDE_SYSTEM &&
        players[p].star < MAX_STAR)
      {
        printf ("hiding %d\n", players[p].star);
        stars[players[p].star].hidden = TRUE;
      }
}

void
update_power ()
{
  int p;

  for (p = 1; p < MAX_PLAYER; p++)
    players[p].powermod = players[p].newpowermod;
}

int
power_rating (struct PLAYER *player)
{
  int result = 0;
  item_sort sort;
  int f;
  
  for (sort = warp_drive; sort <= shield; sort++)
    for (f = 60 ; TRUE ; f *= 2)
      {
        if (factor (sort, player) > f)
          {
            result++;
          }
        else
          {
            if (factor (sort, player) > dice(f))
              result++;
            break;
          }
      }
  for (f = 256 ; f < 2000000 ; f *= 4)
    {
      if (player->energy > f)
        {
          result++;
        }
      else
        {
          if (player->energy > dice(f))
            result++;
          break;
        }
    }
  return (result);
}

int
player_load ()
{
  int t, count, total;
  FILE *fd;
  char buffer[256];
  int num_actives[10];
  snprintf(buffer, 256, "%s/player_load", desired_directory);

  total = 0;
  fd = fopen(buffer, "r");
  if (fd)
    {
      for (t = 0; t < 10; t++)
        {
          fscanf (fd, "%d\n", &count);
          num_actives[t] = count;
        }
    }
  else
    {
      for (t = 0; t < 10 ; t++)
        {
          num_actives[t] = active_players;
        }
    }

  fclose (fd);

  if (really_send)
    {
      fd = fopen(buffer, "w");
      for (t = 1; t < 10; t++)
        {
          fprintf (fd, "%d\n", num_actives[t]);
          total += count;
        }
      fprintf(fd, "%d\n", active_players);
      fclose (fd);
    }

  total += active_players;
  average_players = total/10;
  return average_players;
}

void
read_all_orders ()
{
  int p, player;
  int total_players = 0, total_hits = 0, holidays = 0;
  int moths = 0;
  int shuffle[MAX_PLAYER];

  for (p = 0; p < MAX_PLAYER; p++)
    shuffle[p] = p;
  for (p = 0; p < MAX_PLAYER; p++)
    {
      int a, b, temp;

      a = dice (MAX_PLAYER);
      b = dice (MAX_PLAYER);
      temp = shuffle[a];
      shuffle[a] = shuffle[b];
      shuffle[b] = temp;
    }
  set_default_orders ();
  if (turn == 0)                /* yuchy */
    return;
  for (p = 0; p < num_players; p++)
    {
      player = shuffle[p];
      if (strcmp (players[player].address, "tbg-moderator@asciiking.com"))
        {
          total_players++;
          if (read_orders (player, turn))
            {
              players[player].last_orders = turn;
              total_hits++;
            }
          else
            if (players[player].star == HOLIDAY
                || players[player].star >= MAX_STAR)
              {
                if (mothballed (player))
                  moths++;
                else
                  holidays++;
              }
        }
    }
  active_players = total_hits;
  fprintf (times,
           "<HR><H2>Player Stats</H2>%d notionally active players, %d missed this turn, %d on holiday.\n",
           total_players - moths,
           total_players - total_hits - holidays - moths,
           holidays);
  fprintf (times, "Average number of orders for last 10 turns was %d<P>\n",
           player_load ());
}

void
write_results ()
{
  FILE *fd;
  char buffer[128];
  int player;

  for (player = 0; player < num_players; player++)
    {
      sprintf (buffer, "%s/results/%d/%s%d.b",
               webroot, game, players[player].name, turn);
      printf ("webroot = %s; game = %d; player name = %s; turn = %d\n",
               webroot, game, players[player].name, turn);

      fd = fopen (buffer, "w");
      if (!fd)
        {
          printf ("Can't create results file %s\n", buffer);
          exit (1);
        }
      show_player (fd, players + player);
      fclose (fd);
    }
}

void
merge_results ()
{
  int player;
  char buffer[2000];
  char battle[2000];
  char bufa[2000];
  char bufb[2000];
  char bufc[2000];
  char bufh[2000];
  for (player = 0; player < num_players; player++)
    {
      snprintf (battle, 1000, "%s/results/%d/battle%d_%d",
                webroot, game, session_id, player);
      snprintf (bufa, 1000, "%s/results/%d/%s%d.a",
                webroot, game, players[player].name, turn);
      snprintf (bufb, 1000, "%s/results/%d/%s%d.b",
                webroot, game, players[player].name, turn);
      snprintf (bufc, 1000, "%s/results/%d/%s%d.c",
                webroot, game, players[player].name, turn);
      snprintf (bufh, 1000, "%s/results/%d/%s%d.h",
                webroot, game, players[player].name, turn);
      snprintf (buffer, 1000,
               "touch %s %s %s %s %s", bufh, battle, bufa, bufb, bufc);
      SYSTEM (buffer);
      snprintf (buffer, 1000,
               "cat %s %s %s %s %s %s/results/javascript.html > %s/results/%d/%s%d.html",
               bufh, battle, bufa, bufc, bufb, webroot,
               webroot, game, players[player].name, turn);
      SYSTEM (buffer);
      snprintf (buffer, 1000,
               "%s/bin/sanitize %s/results/%d/%s%d.html %s/results/%d/share_%s%d.html",
               gameroot,
               webroot, game, players[player].name, turn,
               webroot, game, players[player].name, turn);
      SYSTEM (buffer);

      if (!(strcmp (players[player].address, "tbg-moderator@asciiking.com") &&
            strcmp (players[player].address, "none")))
        continue;
      unlink(bufa);
      unlink(bufb);
      unlink(bufc);
      unlink(bufh);
      if (turn && really_send && send_email && players[player].account_number)
        {
          int url_only = players[player].preferences & 128;
          int one_email = players[player].preferences & 1;
          
          snprintf (buffer, 2000,
                   "mail -s \"TBG Turn %d Ready\" %s < %s/secrets/%d/%s ",
                   turn, players[player].address, desired_directory, game, players[player].name);
          SYSTEM (buffer);
          if (url_only)
            continue;
          if (one_email)
            {
              snprintf (buffer, 2000,
                       "HOME=$TBG mail -s \"TBG Turn %d & News\" -a $TBG/results/%d/%s%d.html -a $TBG/results/%d/times%d.html %s < $TBG/secrets/%d/%s",
                       turn, 
                       game, players[player].name, turn, 
                       game,  turn,
                       players[player].address,
                       game, players[player].name);
              printf ("running: %s\n", buffer);
              SYSTEM (buffer);
            }
          else
            {
              snprintf (buffer, 2000,
                       "HOME=$TBG mail -s \"TBG Turn %d\" -a $TBG/results/%d/%s%d.html %s < $TBG/secrets/%d/%s",
                       turn, 
                       game, players[player].name, turn,
                       players[player].address,
                       game, players[player].name);
              printf ("running: %s\n", buffer);
              SYSTEM (buffer);
              snprintf (buffer, 2000,
                       "HOME=$TBG mail -s \"Subspace News %d\" -a $TBG/results/%d/times%d.html %s < $TBG/secrets/%d/%s",
                       turn, 
                       game, turn,
                       players[player].address,
                       game, players[player].name);
              sleep (2);
              printf ("running: %s\n", buffer);
              SYSTEM (buffer);
            }
          sleep (2);
        }
    }
}

char *months[12] = {
  "January", "February", "March",
  "April", "May", "June",
  "July", "August", "September",
  "October", "November", "December"
};

void
open_times ()
{
  char buffer[256];
  char buf2[256];
  FILE *fd;
  int i;
  time_t timer;
  struct tm *stardate;

  sprintf (buffer, "%s/report", desired_directory);
  report = fopen (buffer, "w");
  if (!report)
    {
      printf ("Can't create report file\n");
      exit (1);
    }

  for (i = 0; i < MAX_RUMOUR; i++)
    rumours[i] = 0;
  sprintf (buffer, "%s/results/%d/times%d.html", webroot, game, turn);
  snprintf (buf2, 512, "%s/times.html", webroot);
  force_symlink (buffer, buf2);
  times = fopen (buffer, "w");
  if (!times)
    {
      printf ("Can't open times (%s)\n", buffer);
      exit (1);
    }
  fprintf (times, "<HTML><HEAD><TITLE>Subspace Times</TITLE>\n");
  timer = time (NULL);
  stardate = localtime (&timer);
  fprintf (times, "</HEAD>\n");
  fprintf (times,
           "<BODY BGCOLOR=\"black\" TEXT=\"yellow\" LINK=\"White\" VLINK=\"green\">\n");
  fprintf (times, "<CENTER><H1>Subspace Times</H1>\n");
  timer = (timer + 386380800) / 86400;
  fprintf (times, "<H1>Issue %d - Stardate %ld.%ld</H1>\n",
           turn, timer / 10, timer % 10);
  sprintf (buffer, "%s/edit%d", desired_directory, turn);
  fd = fopen (buffer, "r");
  if (!fd)
    {
      printf ("Can't open editorial\n");
      return;
    }
  while (!feof (fd))
    {
      if (fgets (buffer, 80, fd))
        fprintf (times, buffer);
    }
  fclose (fd);
}

void
generate_times (int code)
{
  int shop, race, player, ad, criminal, loc;

  switch (code)
    {
    case 0:
      shop = dice (MAX_SHOP);
      fprintf (times, "<HR><FONT COLOR=\"CYAN\">\n");
      show_ship (times, shops + shop);
      fprintf (times, "</FONT>\n");
      break;
    case 1:
      race = dice (32);
      if (races[race].plague > 50)
        fprintf (times,
                 "<HR><FONT COLOR=\"RED\">Help! %s Plague getting out of hand at <STRONG>%s</STRONG></FONT>\n",
                 races[race].name, star_names[homeworlds[race]]);
      else
        generate_times (dice (5));
      break;
    case 2:
      race = dice (32);
      fprintf (times,
               "<HR><FONT COLOR=\"ORANGE\"><STRONG>The %s People denounce all their enemies ~ ",
               races[race].name);
      for (player = 0; player < num_players; player++)
        if (players[player].enemies & (1 << race))
          fprintf (times, "%s ~ \n", name_string (players[player].name));
      fprintf (times, "</STRONG></FONT>\n");
      break;
    case 3:
      ad = dice (MAX_ADVENTURE);
      if (ADVENTURE_LEVEL (adventures[ad].parameter) < dice (10))
        fprintf (times,
                 "<HR><FONT COLOR=\"GREY\">Rumours of adventures at <STRONG>%s</STRONG></FONT>\n",
                 star_names[adventures[ad].star]);
      else
        generate_times (dice (5));
      break;
    case 4:
      do
        loc = dice (MAX_LOCATION);
      while (locations[loc].star < 0);
      criminal = locations[loc].criminal & 7;
      if (criminal && criminal < dice (8))
        fprintf (times,
                 "<HR><FONT COLOR=\"GREY\">%s spotted at <STRONG>%s</STRONG></FONT>\n",
                 criminal_string (criminal), star_names[locations[loc].star]);
      else
        generate_times (dice (5));
      break;
    default:
      printf ("Strange rumour number\n");
    }
}

void
close_times ()
{
  int target_rumours;
  int player_rumours = 0;
  int rumour;
  char buffer[256];

  fclose (report);
  sprintf (buffer,
           "mail -s \"TBG: Turn %d Report\" tbg-moderator@asciiking.com <%s/report",
           turn, desired_directory );
  if (really_send)
    SYSTEM (buffer);

  for (rumour = 0; rumour < MAX_RUMOUR; rumour++)
    if (rumours[rumour])
      player_rumours++;
  if (want_verbose)
    printf ("Found %d player rumors\n", player_rumours);
  /* This may not be working...
  target_rumours = 2 + dice(5) + average_players/10;

  for (rumour = 0; rumour < MAX_RUMOUR; rumour++)
    if (rumours[rumour])
      {
        int d = dice(player_rumours);
        while (target_rumours > d)
          {
            generate_times (dice (5));
            d += player_rumours;
            target_rumours--;
          }
        player_rumours--;
        if (rumours[rumour][0] == '!')
          new_ranking (rumours[rumour] + 1);
        else
          fprintf (times,"%s\n", rumours[rumour]);
      }
  while (target_rumours--)
    {
      generate_times (dice (5));
    }
  ...let's try something simpler  */  

  generate_times (dice (5));

  for (rumour = 0; rumour < MAX_RUMOUR; rumour++)
    if (rumours[rumour])
      {
        if (rumours[rumour][0] == '!')
          new_ranking (rumours[rumour] + 1);
        else
          fprintf (times,"%s\n", rumours[rumour]);
      }
  generate_times (dice (5));

  do_rankings (times);


  fprintf (times,
           "<hr><a href=\"http://%s\"><img src=\"http://%s/counter.gif\"></a>\n",
           server, server);

  fprintf (times, "</CENTER></BODY></HTML>\n");
  fclose (times);
}

void
check_items ()
{
  int item, player, adventure;

  for (player = 0; player < MAX_SHIP; player++)
    {
      struct ITEM *it = items + ships[player].ship;

      if (it == items - 1)
        {
          it = items;
          ships[player].ship = 0;
        }

      while (it != items)
        {
          if (it == items - 1)
            printf ("Bad one on %s (%d)\n", ships[player].name, player);
          it->flags |= ITEM_REALLY_IN_USE;
          if (it->link == -1)
            it->link = 0;
          it = it->link + items;
        }
    }
  for (adventure = 0; adventure < MAX_ADVENTURE; adventure++)
    items[adventures[adventure].treasure].flags |= ITEM_REALLY_IN_USE;
  for (item = 0; item < max_item; item++)
    {
      int fixme = 0;
      if (((items[item].flags & ITEM_REALLY_IN_USE) >> 1) !=
          (items[item].flags & ITEM_IN_USE))
        {
          printf ("Bad item %d, flags %d\n", item, items[item].flags);
          fixme = 1;
        }
      if (! (items[item].flags & ITEM_REALLY_IN_USE)
          && items[item].link != 0)
        {
          printf ("Item not in use %d refers to other item %d\n",
                  item, items[item].link);
          items[item].link = 0;
          fixme = 1;
        }
      if (fixme)
        items[item].flags = 0;
    }
}

void
init_units ()
{
  int u;

  for (u = 0; u < MAX_UNIT; u++)
    {
      units[u].sort = u / 8;
      sprintf (units[u].name, "%s %s", star_names[u], unit_types[u / 8].name);
    }
}

void
update_passwds ()
{
  int p;

  for (p = 0; p < MAX_PLAYER; p++)
    {
      players[p].passwd_flags &= ~(1 << password_key);
    }
}

void
shop_stats ()
{
  int shop, i;
  struct ITEM *item;
  int tech[7], modules[7];

  memset (tech, 0, sizeof (tech));
  memset (modules, 0, sizeof (modules));
  for (shop = 0; shop < MAX_SHOP; shop++)
    {
      item = items + shops[shop].ship;
      tech[item->efficiency]++;
      while (item != items)
        {
          modules[item->efficiency]++;
          item = items + item->link;
        }
    }
  for (i = 0; i < 7; i++)
    printf ("%d %s, %d modules\n", tech[i], tech_level_names[i], modules[i]);
}


void
check_flags ()
{
  int player;
  struct ITEM *item;

  for (player = 0; player < MAX_PLAYER; player++)
    {
      item = items + players[player].ship;
      while (item != items)
        {
          if ((item->flags & ITEM_IN_USE) == 0)
            printf ("Bad item %d of %s\n",
                    item - items, players[player].name);
          item->flags |= ITEM_IN_USE;
          item = items + item->link;
        }
    }
}


void
show_rings ()
{
  int loc;
  int p;
  int found = 0;

  for (loc = 0; loc < MAX_LOCATION; loc++)
    if (locations[loc].ring)
      {
        printf ("%s at %s %s\n",
                ring_string (locations[loc].ring),
                star_names[locations[loc].star], loc_string (loc));
        if (found & locations[loc].ring)
          {
            printf ("Duplicate ring!\n");
            locations[loc].ring = 0;
          }
        found |= locations[loc].ring;
      }
  for (p = 1 ; p < MAX_PLAYER ; p++)
    {
      found |= players[p].rings_held;
    }
  if (found != 0xff)
    printf ("Lost one or more rings - mask is %x\n", found);
}

void
make_web_form (FILE * fd, char *label, int code)
{
  int p, k;

  fprintf (fd, "<input type=\"hidden\" name=\"Z\" value=\"%d\">\n", game);
  fprintf (fd, "<input type=\"hidden\" name=\"z\" value=\"%d\">\n", code);
  if (code == -2)
    {
      fprintf (fd,
               "<textarea name=\"n\" rows=\"8\" cols=\"50\">\n\n\n\n\n\n\n\nSigned:\n");
      fprintf (fd, "</textarea>\n");
    }
  fprintf (fd, "<select name=\"t\">\n");

  fprintf (fd, "<option value=\"0\">CHANGE THIS TO SOMETHING ELSE!</option>\n");
  for (p = 1; p < MAX_PLAYER; p++)
    {
      k = sorted_names[p];
      if (players[k].account_number) 
        fprintf (fd, "<option value=\"%d\">%s</option>\n", k, name_string (players[k].name));
    }
  fprintf (fd, "</select>\n");
  fprintf (fd, "<input type=\"submit\" value=\"%s\">\n", label);
  fprintf (fd, "</form>\n");
}

void
make_links ()
{
  int p;
  char buffer[256];
  char buf2[256];

  for (p = 0; p < MAX_PLAYER; p++)
    {
      snprintf (buffer, 256, "%s/%s.htm",
                webroot, uint32_name (players[p].password));
      unlink (buffer);

      snprintf (buffer, 256, "%s/%s.php",
                webroot, uint32_name (players[p].password));
      unlink (buffer);

      snprintf (buffer, 256, "%s/share_%s.htm", webroot,
               uint32_name (public_password(players[p].password)));
      unlink (buffer);

            snprintf (buffer, 256, "%s/results/%d/%s%d.html", webroot,
                game, players[p].name, turn);
      snprintf (buf2, 256, "%s/%s.htm", webroot,
                uint32_name (players[p].password));
      force_symlink (buffer, buf2);

      snprintf (buffer, 256, "%s/turnulator.php", webroot);
      snprintf (buf2, 256, "%s/%s.php", webroot,
                uint32_name (players[p].password));
      force_symlink (buffer, buf2);

      snprintf (buffer, 256,
               "%s/results/%d/share_%s%d.html",
               webroot, game, players[p].name, turn);
      snprintf (buf2, 256,
                "%s/share_%s.htm", webroot,
                uint32_name (public_password(players[p].password)));
      force_symlink (buffer, buf2);


      snprintf (buffer, 256, "%s/orders/%d/%s%d",
               webroot, game,
               players[p].name, turn + 1);
      snprintf (buf2, 256, "%s/%s.ord", webroot, 
               uint32_name (players[p].password));
      unlink(buffer);
      unlink(buf2);
      force_symlink (buffer, buf2);
    }
}


int
cmpname (const void *a, const void *b)
{
  int first = *(int *) a;
  int second = *(int *) b;

  return (strcasecmp (players[first].name, players[second].name));
}

void
sort_names ()
{
  int p;

  if (!sorted_names) 
    { 
    sorted_names = malloc (MAX_PLAYER * sizeof(int)); 
    memset (sorted_names, 0, MAX_PLAYER * sizeof (int)); 
    }

  for (p = 0; p < MAX_PLAYER; p++)
    sorted_names[p] = p;
  qsort (sorted_names + 1, MAX_PLAYER - 1, sizeof (int), cmpname);
}


void
make_web_pages ()
{
  FILE *fd;
  char buffer[256];

  if (really_send)
    make_links ();

  sprintf (buffer, "%s/alias.html", webroot);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't open web page file\n");
      exit (1);
    }
  fprintf (fd, "<html><head><title>TBG Utilities</title>\n");
  fprintf (fd, "</head><body><div style=\"background-color:black;color:#ff8;float:right;\">\n");
  fprintf (fd, "<div id=\"all\">\n<h1>Anonymous Mail</h2>\n");
  fprintf (fd, "<div id=\"anon_mail\">\n");
  fprintf (fd,
           "<form action=\"http://%s/cgi-bin/tbgmail.cgi\" method= \"post\">\n",
           server);
  //fprintf (fd, "<INPUT NAME=\"to\" VALUE=\"tbg@%s\" TYPE=HIDDEN>\n", mail_server);
  make_web_form (fd, "Send mail via server", -2);
  fprintf (fd, "</div><div style=\"margin:1em\">\n");

  fprintf (fd, "<h1>Resending Results and Orders</h1>\n");
  fprintf (fd, "<div id=\"results\">\n");
  fprintf (fd,
           "<form action=\"http://%s/cgi-bin/tbgmail.cgi\" method= \"post\">\n",
           server);
  //fprintf (fd, "<INPUT NAME=\"to\" VALUE=\"tbg@%s\" TYPE=HIDDEN>\n", mail_server);
  make_web_form (fd, "Resend Results", -3);

/*
  fprintf (fd,
           "<FORM ACTION=\"http://%s/cgi-bin/tbgmail.cgi\" METHOD= \"POST\">\n",
           server);
  //fprintf (fd, "<INPUT NAME=\"to\" VALUE=\"tbg@%s\" TYPE=HIDDEN>\n", mail_server);
  make_web_form (fd, "Resend Orders", -4);
*/
  fprintf (fd, "</div><div id=\"resend_url\"\n");
  fprintf (fd,
           "<form action=\"http://%s/cgi-bin/tbgmail.cgi\" method= \"post\">\n",
           server);
  //fprintf (fd, "<INPUT NAME=\"to\" VALUE=\"tbg@%s\" TYPE=HIDDEN>\n", mail_server);
  make_web_form (fd, "Resend Secret URL", -8);

  fprintf (fd, "</div></div></div></body></html>\n");
  fclose (fd);
}

void
make_secrets ()
{
  FILE *fd, *list_file;
  int p;
  char buffer[256];

  sprintf (buffer, "%s/tbg%d_secrets", gameroot, game);
  list_file = fopen (buffer, "w");
  if (!list_file)
    {
      printf ("Can't create secrets listing file!\n");
      exit (1);
    }
  for (p = 1; p < MAX_PLAYER; p++)
    {
      fprintf (list_file, "%d %s\n", players[p].account_number,
               uint32_name (players[p].password));
      sprintf (buffer, "%s/secrets/%d/%s", desired_directory, game,
               players[p].name);
      fd = fopen (buffer, "w");
      if (!fd)
        {
          printf ("Can't create secret file %s\n", buffer);
          exit (1);
        }
      fprintf (fd, "%s,\nYour TBG secret URL is:\n", players[p].name);
      fprintf (fd, "\thttp://%s/%s.htm\n",
               server, uint32_name (players[p].password));
      if (players[p].account_number >= 200)
        fprintf (fd,
                 "Your player web page is:\n\thttp://%s/players/%s_%d.htm\n",
                 server, passwords[players[p].account_number],
                 players[p].account_number);
      fprintf (fd, "Your ranking is %d of %d\n", players[p].ranking,
               MAX_PLAYER);
      fclose (fd);
    }
  fclose (list_file);
}

void
write_new_files ()
{
  FILE *fd;
  char name[256];
  char buffer[256];
  int p;

  sprintf (name, "%s/players", gameroot);
  sprintf (buffer, "%s-%d.bak", name, turn);
  link(name, buffer);
  unlink(name);
  
  fd = fopen (name, "w");
  if (!fd)
    {
      printf ("Can't create master player file!\n");
      exit (1);
    }
  for (p = 0; p < MAX_PLAYER; p++)
    fprintf (fd, "%d %s 0\n", p, players[p].address);
  for (p = 0; p < 26; p++)
    fprintf (fd, "%d tbg-admin@asciiking.com 0\n", p + MAX_PLAYER);
  fclose (fd);
  sprintf (name, "%s/tbg/g%d", gameroot, game);
  fd = fopen (name, "w");
  if (!fd)
    {
      printf ("Can't create new player file!\n");
      exit (1);
    }
  fprintf (fd, "%d\n%d\n", turn, seed);
  for (p = 0; p < MAX_PLAYER; p++)
    fprintf (fd, "%s %d %d 0\n", players[p].name, p, players[p].preferences);
  fclose (fd);
}

void
make_ship_files ()
{
  FILE *fd;
  char name[256];
  int p;

  for (p = 0; p < MAX_SHIP; p++)
    {
      sprintf (name, "%s/results/%d/ship%d_%d.html",
               webroot, game, p, turn);
      fd = fopen (name, "w");
      if (!fd)
        {
          printf ("Can't open ship file\n");
          exit (1);
        }
      fprintf (fd, "<HTML><HEAD><TITLE>%s, Turn %d</TITLE></HEAD>\n",
               name_string (players[p].name), turn);
      fprintf (fd,
               "<BODY TEXT=\"yellow\" BGCOLOR=\"black\" LINK=\"white\" VLINK=\"cyan\"><CENTER>\n");
      show_ship (fd, players + p);
      show_factors (fd, players + p);
      fprintf (fd, "</CENTER></BODY></HTML>\n");
      fclose (fd);
    }
}


void
chrcat (char *p, char c)
{
  char buffer[2] = " ";

  buffer[0] = c;
  strcat (p, buffer);
}

void
write_demographics ()
{
  FILE *fd;
  char table[250][150];
  char buffer[2048];
  int i;

  if (turn > 2490)
    return;

  sprintf (buffer, "%s/demographic.html", webroot);
  fd = fopen (buffer, "w");
  for (i = 0; i < 80; i++)
    table[i][0] = '\0';
  for (i = 1; i < MAX_PLAYER; i++)
    chrcat (table[players[i].last_restart / 10], players[i].name[0]);
  for (i = 0; i < turn / 10; i++)
    fprintf (fd, "<br>%3d-%3d (%d): %s\n",
             i * 10 + 1, (i + 1) * 10, strlen (table[i]), table[i]);
  fclose (fd);
}

const char *
minister_name(int i)
{
  return i ? name_string(players[i].name) : "<em>Vacant</em>";
}

void
show_candidates ()
{
  int p;

  fprintf (times, "<HR><FONT COLOR=\"LIGHTGREEN\">");
  fprintf (times, "<table border=1>\n");
  fprintf (times, "<tr><th colspan=2>The Galactic Council</th></tr>\n");
  fprintf (times, "<tr align=center><td>President</td><td>%s</td></tr>\n",
           minister_name(ministers[PRESIDENT]));
  fprintf (times, "<tr align=center><td>Vice President</td><td>%s</td></tr>\n",
           minister_name (ministers[VEEP]));
  for (p = 0; p < 4; p++)
    fprintf (times, "<tr align=center><td>%s Minister</td><td>%s</td></tr>\n",
             skill_names[p], minister_name (ministers[p]));
  fprintf (times, "<tr align=center><td>Industry Minister</td><td>%s</td></tr>\n",
           minister_name (ministers[MIN_IND]));
  fprintf (times, "<tr align=center><td>Justice Minister</td><td>%s</td></tr>\n",
           minister_name (ministers[MIN_JUST]));

  fprintf (times, "<tr><th colspan=2>Candidates</th></tr>");
  for (p = 1; p < MAX_PLAYER; p++)
    if (players[p].politics & CANDIDATE)
      fprintf (times, "<tr align=center><td colspan=2>%s%s</td></tr>\n",
               name_string (players[p].name),
               players[p].politics & CENSORED ? " (disqualified)" : "");
  fprintf (times, "</table>\n");
  fprintf (times, "</FONT>\n");
}

void
show_plagues ()
{
  int i;

  for (i = 0; i < 32; i++)
    {
      printf ("%s plague %d%%\n", races[i].name, races[i].plague);
    }
}

/* used in experimental dybuk code, now removed, could be useful */

int
count_modules (struct ITEM *base, int sort)
{
  int result = 0;

  while (base != items)
    {
      //              printf("base id is %d, sort is %d\n", base - items, base->sort);
      if (module_type (base->sort) == sort)
        result++;
      base = items + base->link;
    }

  return (result);
}

/* used in experimental dybuk code, now removed, could be useful */

int
get_nth_module (struct ITEM *base, int sort, int n)
{
  while (base != items)
    {
      if (module_type (base->sort) == sort)
        {
          if (n == 0)
            return (base - items);
          else
            n--;
        }
      base = items + base->link;
    }
  return (0);
}

void
make_index ()
{
  int p;
  FILE *fd;
  char fname[1024];
  
  snprintf (fname, sizeof (fname), "%s/results/%d/index.html", webroot, 1);
  
  fd = fopen (fname, "w");
  fprintf (fd, "<html>\n <head>\n  <title>TBG player index</title>\n </head>\n");
  fprintf (fd, " <body>\n  <h1>TBG player index</h1>\n  <table>\n");
  
  for (p = 0 ; p < MAX_PLAYER ; p++)
    {
      if (! mothballed (p))
        {
          fprintf (fd, "   <tr>\n");
          fprintf (fd, "    <td>%d</td>\n", p);
          fprintf (fd, "    <td><a href=\"%s%d.html\">%s</a></td>\n",
                   players[p].name, turn, name_string (players[p].name));
          if (players[p].star < MAX_STAR)
            fprintf (fd, "    <td>%s</td>\n",
                     star_names[players[p].star]);
          else
            fprintf (fd, "    <td>Holiday</td>\n");
            
          fprintf (fd, "   </tr>");
        }
    }
  fprintf (fd, "  </table>\n </body>\n</html>");

}

  

int
jm_main ()
{
  struct stat s;
  if (! gameroot)
    {
      gameroot = getenv("TBG");
    }
  if (!gameroot)
    {
      printf ("Need a game root directory\n");
      printf ("or you must set TBG\n");
      exit (-1);
    }
  stat(gameroot, &s);
  if (! S_ISDIR(s.st_mode))
    {
      fprintf(stderr, "Error: %s not a directory\n", gameroot);
      exit(-1);
    }
  if (!desired_directory)
    {
      char buf[512];
      snprintf(buf, 512, "%s/tbg", gameroot);
      desired_directory = strdup(buf);
    }
  if (!desired_directory)
    {
      printf ("Must set TBG\n");
      exit (-1);
    }
  stat(desired_directory, &s);
  if (! S_ISDIR(s.st_mode))
    {
      fprintf(stderr, "Error: %s not a directory\n", desired_directory);
      exit(-1);
    }

  if (! webroot)
    {
      char buf[512];
      snprintf(buf, 512, "%s/WWW", gameroot);
      webroot = strdup(buf);
    }
  read_master_file ();
  init_units ();
  reset_stars ();
  if (turn)
    {
      open_times ();
      read_data ();
      read_players ();
      write_demographics ();
      check_items ();
      consolidate_artifacts ();
      consolidate_votes ();
      read_all_orders ();
      reset_npcs ();            /* lots of these ? */
      name_shops ();
      update_passwds ();
      consolidate_votes ();
      do_popcorn_auction ();
      execute_orders ();
      do_tribunal_election ();
      do_election ();
      if (dybuk)
        snprintf (dybuk->banner, sizeof (dybuk->banner), "Dybuk of Evil");
/*
      fprintf (times, "<hr>(Debug) Total combat pollution %d\n",
               battle_pollution / 100);
*/
      update_popcorn ();
      make_ship_files ();
      reset_npcs ();
    }
  else
    {
      init_game();
      parse_order ("X", 0);
      reset_npcs ();
    }

  write_results ();
  consolidate_votes ();
  open_times ();
  show_candidates ();
  fprintf (times, "<HR>\n");
  big_ranking (times, -1, "Top Twenty");        // seems to cause memory violation, but do does Kipper block
  all_rankings ();
  close_times ();
  check_flags ();               /* actually repairs them too! */
  show_rings ();
  write_data ();
  make_secrets ();              /* must be before mailing out */
  merge_results ();
  write_master_file ();
  write_players ();
  alien_reports ();
  sort_names ();
  make_web_pages ();
  make_index ();
  show_favour ();
  printf ("End of tbg.c\n");
  return (0);
}

