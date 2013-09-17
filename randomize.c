#include "globals.h"
#include "adventures.h"
#include "criminals.h"
#include "cargo.h"
#include "tbg.h"
#include "data.h"
#include "rand.h"
#include "items.h"
#include "dybuk.h"

#include <string.h>
#include <stdlib.h>

void
tweak_modules (int ship)
{
  struct ITEM *item = items + ship;
  int fixed[4] = { 10,10,10,10 };
  int broken[4] = { 10,10,10,10 };
  int reliability[4] = { 0,0,0,0 };
  
  while (item != items)
    {
      int sort = item->sort;
      int r = repairers[sort];
      if (sort != pod && sort != artifact)
        {
          if (item->flags & ITEM_BROKEN)
            {
              if (rand_exp(fixed[r]) < broken[r] )
                {
                  item->flags &= ~ITEM_BROKEN;
                  fixed[r] += item->efficiency;
                  reliability[r] += item->efficiency* (99 - item->reliability);
                  item->reliability = 99;
                }
            }
          else 
            {
              if (rand_exp(broken[r]) < fixed[r] )
                {
                  int e;
                  item->flags |= ITEM_BROKEN;
                  broken[r] += item->efficiency;
                  e = rand_exp (reliability[r]/(item->efficiency));
                  if (e >= item->reliability)
                    e = item->reliability -1; 
                  item->reliability -= e;
                  reliability[r] -= item->efficiency * e;
                  if (reliability[r] < 0)
                    reliability[r] = 0;
                }
            }
        }
      item = items + item->link;
    }
}

void
randomize_shops ()
{
  int shop;
  struct ITEM *item;
  loot.ship = 0;
  for (shop = 0 ; shop < MAX_SHOP ; shop++)
    {

      item = items + shops[shop].ship;

      while (item != items)
        {
          struct ITEM *temp = items + item->link;
          remove_item (item - items);
          loot.ship = add_item (&loot, item - items);
          item = temp;
        }
      shops[shop].ship = 0;
    }
  item = items + loot.ship;
  while (item != items)
    {
      struct ITEM *temp = items + item->link;
      shop = dice (MAX_SHOP);
      shops[shop].ship = add_item (shops + shop, item - items);
      item = temp;
    }
}

void
randomize_player (struct PLAYER *p)
{
  //  shuffle_bitmap (p->ads, MAX_ADVENTURE);
  //  shuffle_bitmap (p->crims, MAX_CRIMINAL);
  tweak_modules(p->ship);
  if (p->torps)
    p->torps = rand_exp(p->torps);
  
  if (p->star == OLYMPUS || (p->star >= 0 && p->star < MAX_STAR ))
    p->star = dice(MAX_STAR);
  p->old_star = dice(MAX_STAR);
}

void
randomize_criminals ()
{
  int i,j;
  for (i = 0 ; i < MAX_CRIMINAL ; i++)
    {
      for (j = 0 ; j < MAX_LOCATION ; j++)
        {
          if (locations[j].criminal == i)
            {
              locations[j].criminal = 0;
              randomize_criminal (i);
              break;
            }
        }
    }
}

void
randomize_players ()
{
  int i;
  for (i = 0 ; i < MAX_PLAYER ; i++)
    {
      randomize_player(players + i);
    }
}

void
randomize_rings ()
{
  int i, j;
  for (i = 0 ; i < MAX_RING ; i++)
    {
      for (j = 0 ; j < MAX_LOCATION ; j++)
        {
          if (locations[j].ring == i)
            {
              init_ring (i);
              break;
            }
        }
    }
}

void
randomize_aliens ()
{
  int i;
  for (i = 0 ; i < MAX_ALIEN ; i++)
    {
      destroy_ship (aliens + i);
    }
  restock();
}

void
randomize_influence ()
{
  int homes[MAX_LOCATION];
  int colonies[MAX_LOCATION];
  int n_homes = 0;
  int n_colonies = 0;
  int i, j;
  int voter;
  int influence;
  
  for (i = 0 ; i < MAX_LOCATION ; i++)
    {
      switch (locations[i].sort)
        {
        case homeworld:
          homes[n_homes++] = i;
          break;
        case colony:
          colonies[n_colonies++] = i;
          break;
        default:
          break;
        }
    }
  for (i = 0 ; i < n_homes - 1; i++)
    {
      j = i + 1 + dice(n_homes - i);
      voter = locations[homes[j]].voter;
      locations[homes[j]].voter = locations[homes[i]].voter;
      locations[homes[i]].voter = voter;
      influence = rand_exp(locations[homes[j]].influence);
      locations[homes[j]].influence = locations[homes[i]].influence;
      locations[homes[i]].influence = influence;
    }
  for (i = 0 ; i < n_colonies - 1; i++)
    {
      j = i + 1 + dice(n_colonies - i);
      voter = locations[colonies[j]].voter;
      locations[colonies[j]].voter = locations[colonies[i]].voter;
      locations[colonies[i]].voter = voter;
      influence = rand_exp(locations[colonies[j]].influence);
      locations[colonies[j]].influence = locations[colonies[i]].influence;
      locations[colonies[i]].influence = influence;
    }
}

void
randomize_stuff ()
{
  init_rng ();
  reset_popcorn ();
  init_prices ();
  randomize_aliens();
  randomize_adventures();
  randomize_influence();
  randomize_criminals();
  randomize_shops();
  randomize_players();
  randomize_rings ();
}

int
main (int agrc, char *argv[])
{
  
  gameroot = getenv ("TBG");
  desired_directory = strcat(gameroot, "/tbg/");
  game = 1;
  turn = -1;
  read_master_file ();
  read_data ();
  randomize_stuff();
  turn--;
  write_data ();
  exit(0);
}
