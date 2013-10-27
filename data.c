#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "bytes.h"
#include "tbg.h"
#include "defs.h"
#include "globals.h"
#include "rand.h"
#include "tbg-big.h"
#include "locations.h"
#include "util.h"


void
read_data ()
{
  FILE *fd;
  char buffer[256];
  int x, y, star, u;
  int planet, loc;
  int item, player, race;
  int a, b, c, d, e, f, g, h, i, j, k, l;
  uint32 ua, ub, uc, ud, ue, uf, ug, uh;
  int data_version;
  int read_item;
  int old_max_player, old_max_alien, old_max_shop;
  //int max_ship;
  
  snprintf (buffer, sizeof(buffer), "%s/data%d", desired_directory, turn);

  fd = fopen (buffer, "r");
  if (!fd)
    {
      printf ("Can't read data file.\n");
      exit (1);
    }

    fscanf (fd, "%d\n", &a);
  data_version = a;
  if (data_version < 204)
    {
      fscanf (fd, "%d %d %d %d %d\n", &a, &b, &c, &d, &e);
    }
  else if (data_version < 206)
    {
      fscanf (fd, "popcorn: %d %d %d %d %d\n", &a, &b, &c, &d, &e);
    }
  else
    {
      fscanf (fd, "popcorn: %d %d %d %d %d %d %d %d\n",
              &a, &b, &c, &d, &e, &f, &g, &h);
    }    
  if (data_version < 206)
    {
      f = g = turn;
      h = min (c,min(d,e));
    }
  popcorn.star = a;
  //  popcorn.evil_chained = b;
  popcorn.impulse_limit = c;
  popcorn.sensor_limit = d;
  popcorn.shield_limit = e;
  popcorn.last_released = f;
  popcorn.last_collected = g;
  popcorn.reward = h;
  /* star data */
  for (x = 0; x < MAXX; x++)
    for (y = 0; y < MAXY; y++)
      board[x][y] = 0xff;
  for (star = 0; star < MAX_STAR; star++)
    {
      if (data_version < 204)
        fscanf (fd, "%d %d %d\n", &a, &b, &c);
      else
        fscanf (fd, "star: %d %d %d\n", &a, &b, &c);
      if (data_version < 206)
        c = 0;
      stars[star].x = a;
      stars[star].y = b;
      stars[star].instability = c;
      stars[star].ore = star < MAX_HAB_STAR ? 100 : 50;
      stars[star].hidden = FALSE;
      board[stars[star].x][stars[star].y] = star;
    }
  for (loc = 0; loc < MAX_LOCATION; loc++)
    {

      if (data_version < 204)
        fscanf (fd, "%d %d %d %d %d %d %d %d %d\n",
                &a, &b, &c, &d, &e, &f, &g, &h, &i);
      else if (data_version < 205)
        {
          fscanf (fd, "loc: %d %d %d %d %d %d %d %d %d\n",
                  &a, &b, &c, &d, &e, &f, &g, &h, &i);
          j = 0;
        }
      else
        fscanf (fd, "loc: %d %d %d %d %d %d %d %d %d %d\n",
                &a, &b, &c, &d, &e, &f, &g, &h, &i, &j);

      stars[a].locations++;
      stars[a].terrain += location_types[b].range;
      stars[a].loc_mask |= location_types[b].exclusion_bit;
      locations[loc].star = a;
      locations[loc].sort = b;
      locations[loc].parameter = c;
      locations[loc].criminal = d;
      locations[loc].rogues = e;
      locations[loc].risk = f;
      locations[loc].used = FALSE;
      locations[loc].voter = g;
      locations[loc].influence = h;
      locations[loc].ring = i;
      locations[loc].instability=j;
      switch (b)
        {
        case colony:
          locations[loc].votes = 1;
          break;
        case homeworld:
          locations[loc].votes = 6;
          break;
        }
    }
  {
    double avg_t = 0.0;
    int n_terr[7];
    int n = 0;
    double real_t_array[MAX_STAR - OLYMPUS];
    double t_max = 0;
    double t_min = 6;
    double m,b;
    double *t_array = real_t_array - OLYMPUS;
    
    for (n = 0 ; n < 7 ; n++)
      {
        n_terr[n] = 0;
      }
    for (star = OLYMPUS; star < MAX_STAR; star++)
      {
        double t;
        if (star < 0 && star != OLYMPUS)
          continue;
        if (stars[star].locations)
          {
            t = 100 * stars[star].terrain;
            t /= stars[star].locations;
            t /= 100;
          }
        else
          {
            t = 6;
          }
        t_array[star] = t;
        if (t > t_max)
          t_max = t;
        if (t < t_min)
          t_min = t;
      }
    /* adjust raw terrain to be in -0.49 to 6.98 range*/
    b = -0.49;
    m = (6.98 - 0.7)/(t_max - t_min);
    /*  0.7 is an empirical fudge factor to improve the distribution */
    /* printf ("m = %f\nb = %f\n", m, b); */
    for (star = OLYMPUS; star < MAX_STAR; star++)
      {
        double t;
        int terr;
        if (star < 0 && star != OLYMPUS)
          continue;
        t = t_array[star];
        t = t - t_min;
        t = m*t + b;
        terr = (t + 0.5);
        stars[star].terrain = terr;
        n_terr[terr]++;
        avg_t += terr;
      }
#if 0
    for (n = 0 ; n < 7 ; n++)
      {
        printf ("stars with terrain %d: %d\n", n, n_terr[n]);
      }
    printf ("Average terrain: %f\n", avg_t/MAX_STAR);
#endif
  }
  if (data_version >= 204)
    fscanf (fd, "homeworlds: ");
  for (star = 0; star < 32; star++)
    fscanf (fd, "%d ", homeworlds + star);
  fscanf (fd, "\n");

  if (data_version >= 204)
    fscanf (fd, "good_prices: ");
  for (planet = 0; planet < 256; planet++)
    fscanf (fd, "%d ", good_prices + planet);
  fscanf (fd, "\n");

  if (data_version <= 200)
    {
      read_item = OLD_MAX_ITEM; 
      max_item = read_item + 4000;
    }
  else if (data_version < 204)
    {
      fscanf (fd, "%d\n", &read_item);
      max_item = read_item;
    }
  else
    {
      fscanf (fd, "max_item: %d\n", &read_item);
      max_item = read_item;
    }

  items = malloc (max_item * sizeof (struct ITEM));
  memset (items, 0, max_item * sizeof (struct ITEM));

  for (item = 0; item < read_item; item++)
    {
      int dummy_item;
      if (data_version < 204)
        fscanf (fd, "%d %d %d %d %d %d %d %d\n",
                &a, &b, &c, &d, &e, &f, &g, &h);
      else
        fscanf (fd, "item #%d: %d %d %d %d %d %d %d %d\n",
                &dummy_item, &a, &b, &c, &d, &e, &f, &g, &h);
      items[item].link = a;
      if (b < 0)
        {
          /*          printf ("Fixing one\n"); */
          b++;
        }
      items[item].sort = b;
      items[item].efficiency = c;
      items[item].reliability = d;
      items[item].collection = e;
      items[item].price = f;
      items[item].flags = g;
      items[item].magic = h;
    }

  if (data_version < 204)
    {
      old_max_player = 200;
      old_max_alien = 128;
      old_max_shop = 64;
    }
  else 
    {
      fscanf (fd, "ship sizes: %d %d %d\n",
              &old_max_player, &old_max_alien, &old_max_shop);
    }

  /* pull these out somewhere else */
//  max_alien = 256;
//  max_player = 200;
//  max_shop = 64;
  /*
  max_ship = max_player + max_alien + max_shop;

  ships = malloc (max_ship * sizeof (struct PLAYER));
  memset (ships, 0, max_ship * sizeof (struct PLAYER));

  players = ships;
  aliens = ships + max_player;
  shops = ships + max_player + max_alien;
  */
  total_votes = malloc (MAX_PLAYER * sizeof(int));
  memset (total_votes, 0, MAX_PLAYER * sizeof (int));

  for (player = 0; player < MAX_SHIP; player++)
    {
      int dummy_ship;
      if (player < MAX_PLAYER)
        {
          snprintf (players[player].banner,
                    sizeof (players[player].banner),
                    "No Flag Set");
          snprintf (players[player].banner_source,
                    sizeof(players[player].banner_source),
                    "No Flag Set");
          
            
          sprintf (players[player].web_source, "http://%s/", server);
        }
      if (   (player >= old_max_player
              && player < MAX_PLAYER)
          || (player >= MAX_PLAYER + old_max_alien
              && player < MAX_PLAYER + MAX_ALIEN)
          || (player >= MAX_PLAYER + MAX_ALIEN + old_max_shop
              && player < MAX_PLAYER + MAX_ALIEN + MAX_SHOP))
        {
          /* ensure shops/aliens are properly reset */
          players[player].ship = 0;
          continue;
        }
      if (data_version < 204)
        fscanf (fd, "%d %d %d %d %d %d %d %d %d %d %d %d\n",
                &a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l);
      else
        fscanf (fd, "ship #%d: %d %d %d %d %d %d %d %d %d %d %d %d\n",
                &dummy_ship, &a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l);
      players[player].ship = a;
      players[player].skills[0] = b;
      players[player].skills[1] = c;
      players[player].skills[2] = d;
      players[player].skills[3] = e;
      players[player].energy = f;
      //              if (g == NOWHERE && player > 0)
      //                      g = player & 63;
      // EEM: move ships to their own holiday planet
      if (g == HOLIDAY)
        g = player + MAX_STAR;
      players[player].star = g;
      
      players[player].movement = g;     /* default to not moving */
      if (h >= MAX_STAR || h == NOWHERE)
        {
          h = player & 63;
          /*          printf ("old_star wrong for %s, changing to %d\n",
                      players[player].name, h); */
        }
      players[player].old_star = h;
      players[player].friends = i;
      players[player].enemies = j;
      players[player].alliance = k;
      players[player].popcorn = l;
      players[player].artifacts = 0;
      players[player].blessings = 0;
      players[player].reports = 0;
      players[player].trib = 0;
      fscanf (fd, "%d %d %d %d %d %d %d %d %d %d %d %d\n",
              &a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l);
      players[player].health = a;
      players[player].crew[0] = b;
      players[player].crew[1] = c;
      players[player].crew[2] = d;
      players[player].crew[3] = e;
      players[player].pools[0] = f;
      players[player].pools[1] = g;
      players[player].pools[2] = h;
      players[player].pools[3] = i;
      players[player].rings_seen = j;
      players[player].rings_held = k;
      players[player].last_orders = l;
      players[player].popcorn_sales = 0;
      players[player].gift = 0;
      fscanf (fd, "%x %x %x %x %x %x %x %x %d\n",
              &ua, &ub, &uc, &ud, &ue, &uf, &ug, &uh, &i);
      players[player].ads[0] = ua;
      players[player].ads[1] = ub;
      players[player].ads[2] = uc;
      players[player].ads[3] = ud;
      players[player].ads[4] = ue;
      players[player].ads[5] = uf;
      players[player].ads[6] = ug;
      players[player].ads[7] = uh;
      players[player].allies = i;
      fscanf (fd, "%x %x %x %x %x %x %x %x %d %d\n",
              &ua, &ub, &uc, &ud, &ue, &uf, &ug, &uh, &i, &j);
      players[player].crims[0] = ua;
      players[player].crims[1] = ub;
      players[player].crims[2] = uc;
      players[player].crims[3] = ud;
      players[player].crims[4] = ue;
      players[player].crims[5] = uf;
      players[player].crims[6] = ug;
      players[player].crims[7] = uh;
      players[player].prisoner = i;
      if (data_version < 203)
        {
          int r = j / 100;
          int v = j % 100;
          j = 1000*r+v;
        }
      players[player].medicine = j;
        
      fscanf (fd, "%d %d %d %d %d %d %d %d %d %d %d %d\n",
              &a, &b, &c, &d, &e, &f, &g, &h, &i, &j, &k, &l);
      players[player].experience[0] = a;
      players[player].experience[1] = b;
      players[player].experience[2] = c;
      players[player].experience[3] = d;
      players[player].favour[0] = e;
      players[player].favour[1] = f;
      players[player].favour[2] = g;
      players[player].favour[3] = h;
      players[player].torps = i;
      players[player].probe = j;
      players[player].dybuk_target = k;
      players[player].last_demon = l;
      players[player].next_demon = FALSE;
      players[player].pillage = -1;
      players[player].strategy.firing_rate = 0;
      players[player].results = FALSE;  /* no file started */
      players[player].got_some_orders = FALSE;
      players[player].restarting = 0;
      players[player].standby = 0;
      players[player].hide_hunt = 0;
      players[player].pollution = 0;
      players[player].bid = 0;
      for (i = 0; i < 4; i++)
        {
          players[player].away_team[i] = -1;
        }
      fscanf (fd, "%d %d %d %d %d %d %d %d %d\n",
              &a, &b, &c, &d, &e, &f, &g, &h, &i);
      players[player].politics = a;
      players[player].passwd_flags = b;
      players[player].chosen = c;
      players[player].tracer = d;
      players[player].companion = e;
      players[player].viewing_trace = FALSE;
      players[player].last_restart = f;
      players[player].flags = g;
      players[player].password = h;
      players[player].heretic = i;
      players[player].denounced = 0;
      players[player].magic_flags = 0;
      if (h == 0)
        players[player].password = rand32();
      if (data_version < 206)
        {

          skill_sort skill;
          fscanf (fd, "%d %d %d %d\n", &a, &b, &c, &d);
          e = 100;
          for (skill = engineering ; skill <= weaponry ; skill++)
            if (players[player].chosen & (1 << skill))
              e = 0;
        }
      else
        fscanf (fd, "%d %d %d %d %d\n", &a, &b, &c, &d, &e);
      players[player].powermod = a;
      players[player].lender = b;
      players[player].sponsor = c;
      players[player].trib = d;
      players[player].evil = e;
      players[player].x_from[0] = '\0';
      if (data_version < 206)
        { 
          for (i = 0 ; i < MAX_STAR/32 ; i++)
            {
              players[player].stars[i] = ~0;
            }
        }
      else
        {
          for (i = 0 ; i < MAX_STAR/32 ; i++)
            {
              fscanf (fd, "%x ", players[player].stars + i);
            }
          fscanf (fd, "\n");
        }
    }
  for (loc = 0; loc < MAX_ADVENTURE; loc++)
    {
      int dummy_loc;
      if (data_version < 204)
        fscanf (fd, "%d %d %d %d %d\n", &a, &b, &c, &d, &e);
      else
        fscanf (fd, "adventure #%d: %d %d %d %d %d\n",
                &dummy_loc, &a, &b, &c, &d, &e);
      adventures[loc].star = a;
      adventures[loc].treasure = b;
      adventures[loc].loc = c;
      adventures[loc].obscurity = d;
      adventures[loc].bonus_flags = e;
      adventures[loc].parameter = loc;
    }
  for (race = 0; race < 32; race++)
    {
      if (data_version < 204)
        fscanf (fd, "%d %d\n", &a, &b);
      else
        fscanf (fd, "race: %d %d\n", &a, &b);
      races[race].wealth = a;
      races[race].plague = b;
    }

  if (data_version >= 204)
    fscanf (fd, "mercs: ");
  for (u = 0; u < MAX_UNIT; u++)
    {
      fscanf (fd, "%d ", &a);
      units[u].pay = a;
    }
  fscanf (fd, "\n");

  if (data_version < 204)
    fscanf (fd, "%d %d %d\n", &a, &b, &c);
  else
    fscanf (fd, "password: %d %d %d\n", &a, &b, &c);
  password_true = a;
  password_false = b;
  password_key = c;
  if (data_version < 204)
    fscanf (fd, "%d %d %d %d\n", &a, &b, &c, &d);
  else
    fscanf (fd, "prophets: %d %d %d %d\n", &a, &b, &c, &d);
  prophets[0] = a;
  prophets[1] = b;
  prophets[2] = c;
  prophets[3] = d;
  if (data_version < 204)
    fscanf (fd, "%d %d %d %d %d %d %d %d %d\n",
            &a, &next_item,
            &next_unit, &next_contract, &next_fee,
            &accuser, &defendent, &dybuk_shields, &dybuk_target);
  else
    fscanf (fd, "update: %d %d %d %d %d %d %d %d %d\n",
            &a, &next_item,
            &next_unit, &next_contract, &next_fee,
            &accuser, &defendent, &dybuk_shields, &dybuk_target);
  case_winner = defendent;
  if (data_version < 206)
    {
      if (a >= 0)
        a = MAX_PLAYER + MAX_ALIEN - 1;
      else
        a = 0;
    }
  dybuk = (a) ? (players + a) : 0;
  if (dybuk)
    {
      snprintf (dybuk->banner,
                sizeof (dybuk->banner),
                "Dybuk of Chaos");
      snprintf (dybuk->banner_source,
                sizeof(dybuk->banner_source),
                "Dybuk of Chaos");
    }

    
  if (data_version >= 204)
    fscanf (fd, "favours: ");
  for (i = 0; i < 4; i++)
    fscanf (fd, "%d ", old_favours + i);
  fscanf (fd, "\n");

  if (data_version >= 204)
    fscanf (fd, "ministers: ");
  for (i = 0; i < 9; i++)
    {
      fscanf (fd, "%d ", ministers + i);
    }
  fscanf (fd, "\n");

  if (data_version >= 204)
    fscanf (fd, "techs: ");
  for (i = 0; i < 7; i++)
    {
      fscanf (fd, "%d ", proposed_techs + i);
    }
  fscanf (fd, "\n");


  if (data_version >= 204)
    fscanf (fd, "types: ");
  for (i = 0; i < 7; i++)
    {
      fscanf (fd, "%d ", proposed_types + i);
    }
  fscanf (fd, "\n");

  for (i = 0; i < 7; i++)
    {
      if (proposed_techs[i] < 1 || proposed_techs[i] > 6)
        proposed_techs[i] = 1 + dice(6);
    }

  if (data_version < 204)
    fscanf (fd, "%d %d\n", &restock_item, &restock_tech);
  else
    fscanf (fd, "restock: %d %d\n", &restock_item, &restock_tech);

  if (data_version < 206)
    {
      for (i = 0 ; i < MAX_STAR/32 ; i++)
        {
          public_stars[i] = ~0;
          evil_stars[i] = ~0;
        }
    }
  else
    {
      fscanf(fd, "public_stars:");
      for (i = 0 ; i < MAX_STAR/32 ; i++)
        {
          fscanf (fd, " %x", public_stars + i);
        }
      fscanf (fd, "\n");

      fscanf(fd, "evil_stars:");
      for (i = 0 ; i < MAX_STAR/32 ; i++)
        {
          fscanf (fd, " %x", evil_stars + i);
        }
      fscanf (fd, "\n");
    }
    
  current_unit = next_unit;
  current_contract = next_contract;
  next_unit = dice (MAX_UNIT);
  if (units[next_unit].pay)     /* already in service */
    {
      for (i = 0; i < MAX_UNIT; i++)
        if (units[i].pay)
          units[i].pay++;
      if (times)
        fprintf (times,
                 "<hr><div class=\"mercs\">Mercenary Shortage, all units' pay increased by $1</div>\n");
      next_unit = -1;
    }

  do
    next_contract = dice (MAX_LOCATION);
  while ((location_types[locations[next_contract].sort].flags & LOC_MERC) == 0
         || locations[next_contract].star < 0);

  for (player = MAX_OLD_PLAYER; player < MAX_PLAYER; player++)
    init_new_player (players + player, 1);
  for (i = 0; i < 9; i++)
    {
      if (i == PRESIDENT || i == VEEP || i == TRIBUNE)
        continue;
      int p = ministers[i];
      if (players[p].politics & CENSORED)
        ministers[i] = 0;
    }
  strcpy (loot.name, "Loot");
  strcpy (loot.banner, "");
  fclose (fd);
  for (i = 0; i < MAX_PLAYER; i++)
    {
      total_votes[i] = 0;
    }
  restock_rogues_race = dice (MAX_RACE);
  restock_rogues_skill = 1;
  printf("Read data file.\n");
}

void
write_data ()
{
  FILE *fd;
  char buffer[128];
  int star, i, u;
  int planet, loc;
  int item, player;
  int count = 0;
  //int max_ship = max_player + max_alien + max_shop;

  sprintf (buffer, "%s/data%d", desired_directory, turn + 1);

  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create data file\n");
      exit (1);
    }
  fprintf (fd, "%d\n", 206); // data version
  fprintf (fd, "popcorn: %d %d %d %d %d %d %d %d\n",
           popcorn.star, 0, popcorn.impulse_limit,
           popcorn.sensor_limit, popcorn.shield_limit,
           popcorn.last_released, popcorn.last_collected,
           popcorn.reward);
  /* star data */
  for (star = 0; star < MAX_STAR; star++)
    {
      fprintf (fd, "star: %d %d %d\n",
               stars[star].x, stars[star].y, stars[star].instability);
    }
  for (loc = 0; loc < MAX_LOCATION; loc++)
    {
      if (locations[loc].rogues ||
          (loc_type (loc, LOC_ROGUE) && dice (128) == 0))
        locations[loc].rogues |= 1;
      fprintf (fd, "loc: %d %d %d %d %d %d %d %d %d %d\n",
               locations[loc].star,
               locations[loc].sort,
               locations[loc].parameter,
               locations[loc].criminal,
               locations[loc].rogues,
               locations[loc].risk,
               locations[loc].voter,
               locations[loc].influence,
               locations[loc].ring,
               locations[loc].instability);
    }
  fprintf (fd, "homeworlds: ");
  for (star = 0; star < 32; star++)
    fprintf (fd, "%d ", homeworlds[star]);
  fprintf (fd, "\n");

  fprintf (fd, "good_prices: ");
  for (planet = 0; planet < 256; planet++)
    fprintf (fd, "%d ", good_prices[planet]);
  fprintf (fd, "\n");

  fprintf (fd, "max_item: %d\n", max_item + extra_items);
  for (item = 0; item < max_item; item++)
    {
      if (!(items[item].flags & ITEM_IN_USE))
        count++;
    }
  printf ("There are %d items free\n", count);
  for (item = 0; item < max_item; item++)
    {
      fprintf (fd, "item #%d: %d %d %d %d %d %d %d %d\n",
               item,
               items[item].link,
               items[item].sort,
               items[item].efficiency,
               items[item].reliability,
               items[item].collection,
               items[item].price,
               items[item].flags & ITEM_SAVE_FLAGS,
               items[item].magic);
    }
  fprintf (fd, "ship sizes: %d %d %d\n", MAX_PLAYER, MAX_ALIEN, MAX_SHOP);
  for (player = 0; player < MAX_SHIP; player++)
    {
      fprintf (fd, "ship #%d: %d %d %d %d %d %d %d %d %d %d %d %d\n",
               player, 
               players[player].ship,
               players[player].skills[0],
               players[player].skills[1],
               players[player].skills[2],
               players[player].skills[3],
               players[player].energy,
               players[player].star,
               players[player].old_star,
               players[player].friends,
               players[player].enemies,
               players[player].alliance, players[player].popcorn);
      fprintf (fd, "%d %d %d %d %d %d %d %d %d %d %d %d\n",
               players[player].health,
               players[player].crew[0],
               players[player].crew[1],
               players[player].crew[2],
               players[player].crew[3],
               players[player].pools[0],
               players[player].pools[1],
               players[player].pools[2],
               players[player].pools[3],
               players[player].rings_seen,
               players[player].rings_held, players[player].last_orders);
      fprintf (fd, "%#010x %#010x %#010x %#010x %#010x %#010x %#010x %#010x %d\n",
               players[player].ads[0],
               players[player].ads[1],
               players[player].ads[2],
               players[player].ads[3],
               players[player].ads[4],
               players[player].ads[5],
               players[player].ads[6],
               players[player].ads[7], players[player].allies);
      fprintf (fd, "%#010x %#010x %#010x %#010x %#010x %#010x %#010x %#010x %d %d\n",
               players[player].crims[0],
               players[player].crims[1],
               players[player].crims[2],
               players[player].crims[3],
               players[player].crims[4],
               players[player].crims[5],
               players[player].crims[6],
               players[player].crims[7],
               players[player].prisoner, players[player].medicine);
      fprintf (fd, "%d %d %d %d %d %d %d %d %d %d %d %d\n",
               players[player].experience[0],
               players[player].experience[1],
               players[player].experience[2],
               players[player].experience[3],
               players[player].favour[0],
               players[player].favour[1],
               players[player].favour[2],
               players[player].favour[3],
               players[player].torps,
               players[player].probe,
               players[player].dybuk_target, players[player].next_demon);
      fprintf (fd, "%d %d %d %d %d %d %d %d %d\n",
               players[player].politics,
               players[player].passwd_flags,
               players[player].chosen,
               players[player].tracer,
               players[player].companion,
               players[player].last_restart,
               players[player].flags,
               players[player].password, players[player].heretic);
      fprintf (fd, "%d %d %d %d %d\n",
               players[player].powermod,
               players[player].lender,
               players[player].sponsor,
               players[player].trib,
               players[player].evil);

      for (i = 0 ; i < MAX_STAR/32 ; i++)
        {
          fprintf (fd, "%#010x ", players[player].stars[i]);
        }
      fprintf (fd, "\n");
    }
  for (loc = 0; loc < MAX_ADVENTURE; loc++)
    {
      fprintf (fd, "adventure #%d: %d %d %d %d %d\n",
               loc,
               adventures[loc].star,
               adventures[loc].treasure,
               adventures[loc].loc,
               adventures[loc].obscurity, adventures[loc].bonus_flags);
    }

  for (player = 0; player < 32; player++)
    {
      fprintf (fd, "race: %d %d\n", races[player].wealth, races[player].plague);
    }
  fprintf (fd, "mercs: ");
  for (u = 0; u < MAX_UNIT; u++)
    fprintf (fd, "%d ", units[u].pay);
  fprintf (fd, "\n");

  fprintf (fd, "password: %d %d %d\n",
           password_true, password_false, password_key);
  fprintf (fd, "prophets: ");
  for (i = 0; i < 4; i++)
    fprintf (fd, "%d ", prophets[i]);
  fprintf (fd, "\n");
  fprintf (fd, "update: %d %d %d %d %d %d %d %d %d\n",
           dybuk? dybuk - players : 0,
           next_item, next_unit, next_contract, next_fee,
           accuser, defendent, dybuk_shields, dybuk_target);

  fprintf (fd, "favours: ");
  for (i = 0; i < 4; i++)
    fprintf (fd, "%d ", favours[i]);
  fprintf (fd, "\n");

  fprintf (fd, "ministers: ");
  for (i = 0; i < 9; i++)
    fprintf (fd, "%d ", ministers[i]);
  fprintf (fd, "\n");

  fprintf (fd, "techs: ");
  for (i = 0 ; i < 7 ; i++)
    fprintf (fd, "%d ", proposed_techs[i]);
  fprintf (fd, "\n");

  fprintf (fd, "types: ");
  for (i = 0 ; i < 7 ; i++)
    fprintf (fd, "%d ", proposed_types[i]);
  fprintf (fd, "\n");

  fprintf (fd, "restock: %d %d\n",
           proposed_types[VEEP],
           proposed_techs[VEEP]);
  fprintf (fd, "public_stars:");
  for (i = 0 ; i < MAX_STAR/32 ; i++)
    {
      fprintf (fd, " %#010x", public_stars[i]);
    }
  fprintf (fd, "\n");

  fprintf (fd, "evil_stars:");
  for (i = 0 ; i < MAX_STAR/32 ; i++)
    {
      fprintf (fd, " %#010x", evil_stars[i]);
    }
  fprintf (fd, "\n");
  fclose (fd);
}


