#include "globals.h"
#include "defs.h"
#include "items.h"
#include "rand.h"
#include "cargo.h"

int
who_home (int star)
{
  int h;

  for (h = 0; h < 32; h++)
    if (homeworlds[h] == star)
      return (h);
  return (NOT_HOMEWORLD);       /* no-one's homeworld */
}

void
move_aliens ()
{
  int alien, race;

  for (alien = 0; alien < MAX_ALIEN; alien++)
    {
      race = aliens[alien].alliance;
      if (dybuk - players == MAX_PLAYER + alien)
        {
          int s, n = 0;
          do
            {
              s = dice (MAX_STAR);
              n++;
            }
          while (!get_bit (evil_stars, s) && n < 20);
          set_bit (evil_stars, s);
          aliens[alien].star = s;
        }
      else if (dice (10) < 4)
        aliens[alien].star = homeworlds[race];
      else if (races[race].hostility == friendly)
        aliens[alien].star = dice (MAX_HAB_STAR);
      else
        aliens[alien].star = dice (MAX_STAR);
    }
}


void
restock_alien (int i)
{
  int race, good;
  int bonus, tech;
  int base, extra;
  int t;
  int average;
  item_sort sort;
  int new;
  int demo = 0;
  struct PLAYER *ship = aliens + i;
  int j;
  
  race = ship->alliance = i & 31;
  tech = races[race].tech_level + dice (2) - dice (2);
  tech = (tech < 1) ? 1 : tech;

  t = tech-1;
  average = 2*t*t;
  extra = (50 - average)/3;
  extra = (2*extra > average) ? average/2 : extra;
  base = average - extra;
  bonus = base + rand_exp (extra);
  bonus = (bonus > 2 * average) ? 2 * average : bonus; //Restored cb
  printf("Alien %s #%d: tech = %d, average = %d, base = %d, extra = %d, bonus = %d\n",
         races[race].name, i, tech, average, base, extra, bonus);
  
  new = new_item (warp_drive, tech, 95, 2, demo);
  if (new < 0)
    {
      ship->ship = 0;
      return;
    }
  ship->ship = new;
  for (sort = impulse_drive; sort <= shield; sort++)
    {
      ship->ship = add_item (ship, new_item (sort, tech, 95, 2, demo));
    }
  /* One weapon guaranteed */
  switch (races[race].style)
    {
    case pirate:
      ship->ship = add_item (ship, new_item (laser, tech, 95, 2, demo));
      break;
    case trader:
      ship->ship = add_item (ship, new_item (drone, tech, 95, 2, demo));
      break;
    case archer:
      ship->ship = add_item (ship, new_item (fighter, tech, 95, 2, demo));
      break;
    case sneaky:
      ship->ship = add_item (ship, new_item (ram, tech, 95, 2, demo));
      break;
    case balanced:
      ship->ship = add_item (ship, new_item (drone, tech, 95, 2, demo));
      break;
    }

  j = bonus;
  while (j--) // Engineering
    {
      int s = -1;
      switch (races[race].style)
        {
        case pirate: // Many impulses
          s = impulse_drive;
          break;
        case archer:
        case sneaky: // Some impulses
          switch (dice (3))
            {
            case 0:
              s = impulse_drive;
              break;
            default:
              break;
            }
          break;
        case trader: // Many warps, some impulses
          switch (dice (3))
            {
            case 0:
            case 1:
              s = warp_drive;
              break;
            case 2:
            default:
              s = impulse_drive;
              break;
            }
          break;
        case balanced: // Many warps and impulses
          switch (dice (2))
            {
            case 0:
              s = impulse_drive;
              break;
            case 1:
            default:
              s = warp_drive;
              break;
            }
          break;
        default: // Shouldn't occur
          break;
        }
      if (s >= 0)
        ship->ship = add_item (ship, new_item (s, tech, 95, 2, demo));
    }
  j = bonus;
  while (j--)  // Science
    {
      int s = -1;
      switch (races[race].style)
        {
        case pirate: // Some sensors and cloaks
          switch (dice (3))
            {
            case 0:
              s = sensor;
              break;
            case 1:
              s = cloak;
              break;
            case 2:
            case 3:
            default:
              break;
            }
          break;
        case archer:  // sensors!!
          s = sensor;
          break;
        case sneaky: // cloaks!
          s = cloak;
          break;
        case trader: 
        case balanced: // Many sensors and cloaks
          switch (dice (2))
            {
            case 0:
              s = sensor;
              break;
            case 1:
            default:
              s = cloak;
              break;
            }
          break;
        default: // Shouldn't occur
          break;
        }
      if (s >= 0)
        ship->ship = add_item (ship, new_item (s, tech, 95, 2, demo));
    }
  j = bonus;
  while (j--) // Medical
    {
      int s = -1;
      switch (races[race].style)
        {
        case pirate: 
        case archer:
        case sneaky: // No Medical modules
          break;
        case trader: // Some medical modules
          switch (dice (4))
            {
            case 0:
              s = sick_bay;
              break;
            case 1:
              s = life_support;
              break;
            case 2:
            default:
              break;
            }
          break;
        case balanced: // Many of both
          switch (dice (2))
            {
            case 0:
              s = sick_bay;
              break;
            case 1:
            default:
              s = life_support;
              break;
            }
          break;
        default: // Shouldn't occur
          break;
        }
      if (s >= 0)
        ship->ship = add_item (ship, new_item (s, tech, 95, 2, demo));
    }
  j = bonus;
  while (j--) // Weaponry
    {
      int s = -1;
      switch (races[race].style)
        {
        case pirate:
          switch (dice(6))
            {
            case 0: // 3 mid range : 3 shields
              s = disruptor;
              break;
            case 1:
              s = laser;
              break;
            case 2:
              s = missile;
              break;
            case 3:
            case 4:
            case 5:
            default:
              s = shield;
              break;
            }
          break;
        case archer: // 3 long range : 2 shields
          switch (dice(5))
            {
            case 0:
              s = missile;
              break;
            case 1:
              s = drone;
              break;
            case 2:
              s = fighter;
              break;
            case 3:
            case 4:
            default:
              s = shield;
              break;
            }
          break;
        case sneaky: // 3 Short range : 1 shields;
          switch (dice(4))
            {
            case 0:
              s = ram;
              break;
            case 1:
              s = gun;
              break;
            case 2:
              s = disruptor;
              break;
            case 3:
            default:
              s = shield;
              break;
            }
          break;
        case trader: // Shields!!!  and some weapons
          switch (dice (15))
            {
            case 0:
            case 1:
              s = fighter;
              break;
            case 2:
              s = drone;
              break;
            case 3:
              s = laser;
              break;
            case 4:
              s = gun;
              break;
            default: // and cases 5-14 for 2 shields/weapon
              s = shield;
            }
          break;
        case balanced: // Shields, and a mix of weapons 1 shield: 1 weapon
          switch (dice (6))
            {
            case 0:
              s = drone;
              break;
            case 1:
              s = laser;
              break;
            case 2:
              s = gun;
              break;
            case 3:
            case 4:
            case 5:
            default:
              s = shield;
              break;
            }
          break;
        default: // Shouldn't occur
          break;
        }
      if (s >= 0)
        ship->ship = add_item (ship, new_item (s, tech, 95, 2, demo));
    }
  switch (races[race].style)
    {
    case pirate:
    case sneaky:
    case archer: // few pods
      j = rand_exp (bonus)/30;
      if (j < 1)
        j = 1;
      break;
    case balanced: // some pods
      j = rand_exp (bonus)/10;
      if (j < 1)
        j = 1;
      break;
    case trader: // many pods
      j = rand_exp (bonus)/3;
      if (j < 2)
        j = 2;
      break;
    default:
      break;
    }
  while(j--)
    {
      ship->ship = add_item (ship, new_item (pod, tech, 95, 2, 0));
      good = dice (25);
      load_pod (items + aliens[i].ship, good, 1);
      load_pod (items + aliens[i].ship, good, 1);
      load_pod (items + aliens[i].ship, good, 1);
      load_pod (items + aliens[i].ship, good, 1);
      load_pod (items + aliens[i].ship, good, 1);
      load_pod (items + aliens[i].ship, good, 1);
      break;
    }

  if (races[race].tech_level > dice(4))
    transfer_item(new_item(artifact, 0, 0, 0, 0),
                  aliens + i);
}

void
restock ()
{
  int i;
  struct ITEM *item;

  for (i = 0; i < MAX_ALIEN; i++)
    {
      item = items + aliens[i].ship;
      while (item != items)
        {
          if (item->flags & ITEM_DEMO)
            destroy_item (item - items);
          item = items + item->link;
        }
      if (aliens[i].ship)
        continue;
      /* replace destroyed ones */
      restock_alien(i);
    }
}

