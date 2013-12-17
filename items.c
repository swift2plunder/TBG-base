#include <stdlib.h>
#include <string.h>
#include "defs.h"
#include "items.h"
#include "globals.h"
#include "rand.h"
#include "util.h"
#include "skill.h"
#include "tbg-big.h"
#include "politics.h"

int
find_free_item ()
{
  int item;
  struct ITEM *new_items;
  
  for (item = 1; item < max_item; item++)
    if ((items[item].flags & ITEM_IN_USE) == 0)
      return (item);

  new_items = realloc (items, (1000 + max_item) * sizeof(struct ITEM));
  if (! new_items)
    {
      printf ("find_free_item: realloc failled\n");
      exit (-1);
    }
  items = new_items;
  memset (items+max_item, '\0', 1000 * sizeof (struct ITEM));
  item = max_item;
  max_item += 1000;
  
  return (item);
}

short
new_item (short sort, short efficiency, short reliability, short collection,
          short demo)
{
  int blessings, curses, keys, bits;
  int item;

  item = find_free_item ();
  if (item == NO_ITEM)
    {
      return item;
    }
  items[item].sort = sort;
  items[item].efficiency = efficiency;
  items[item].reliability = reliability;
  items[item].collection = collection;
  if (sort == artifact)
    {
      do
        {
          keys = 0;
          bits = dice (3) + 1;
          while (bits--)
            keys |= 1 << dice (8);
          blessings = 1 << dice (8);
          curses = 0;
          bits = dice (3) + 1;
          while (bits--)
            curses |= 1 << dice (8);
        }
      while (blessings & curses);
      items[item].magic = 
        (dice (256) << 24) + (blessings << 16) + (curses << 8) + keys;
    }
  else
    items[item].magic = 0;
  items[item].price = 50 * (1 << items[item].efficiency);
  items[item].flags = ITEM_IN_USE;
  if (demo)
    items[item].flags |= ITEM_DEMO;
  items[item].link = 0;
  if (sort == pod)
    {
      items[item].reliability = 0;
      items[item].collection = 0;
    }
  return (item);
}



/* connect 'new' into chain starting at 'base', keeping it sorted,
   returns new value for base in case it had to sort first */
short
add_item (struct PLAYER *ship, short new)
{
  short previous, next, base;

  base = ship->ship;
  if (new == NO_ITEM)
    {
      return base;
    }

  if (base == 0)
    {
      ship->ship = new;
      items[new].link = 0;
      return (new);
    }

  if (items[base].sort > items[new].sort)
    {
      /* sort first, moving base */
      items[new].link = base;
      ship->ship = new;
      return (new);
    }

  if (   items[new].sort == evil_artifact
      && items[base].sort == evil_artifact)
    {
      skill_sort sk;
      for (sk = engineering ; sk <= medical; sk++)
        {
          int a = (items[next].magic & (0x03ff << (10 * sk))) >> (10*sk);
          int b = (items[new].magic & (0x03ff << (10 * sk))) >> (10*sk);
          int bombs = a + b;
          if (bombs > 0x03ff)
            bombs = 0x03ff;
          items[base].magic &= ~(0x03ff << (sk * 10));
          items[base].magic |= ((bombs & 0x03ff) << (sk * 10));
        }
      destroy_item(new);
      return base;
    }

  if (items[base].link == 0)    /* trivial one item list */
    {
      items[base].link = new;
      items[new].link = 0;
      return (base);
    }

  next = base;
  while (items[next].link)
    {
      previous = next;
      next = items[next].link;
      if (   items[new].sort == evil_artifact
             && items[next].sort == evil_artifact)
        {
          skill_sort sk;
          for (sk = engineering ; sk <= medical; sk++)
            {
              int a = (items[next].magic & (0x03ff << (10 * sk))) >> (10*sk);
              int b = (items[new].magic & (0x03ff << (10 * sk))) >> (10*sk);
              int bombs = a + b;
              if (bombs > 0x03ff)
                bombs = 0x03ff;
              items[next].magic &= ~(0x03ff << (sk * 10));
              items[next].magic |= ((bombs & 0x03ff) << (sk * 10));
            }
          destroy_item(new);
          return base;
        }
      if (items[next].sort > items[new].sort)
        {
          items[new].link = items[previous].link;
          items[previous].link = new;
          return (base);
        }
    }
  items[next].link = new;
  items[new].link = 0;
  return (base);
}

short
generate_item (int tech, int demo)
{
  return (new_item (dice (15), tech, 50 + dice (50), dice (3) + 1, demo));
}

short
find_owner (short item)
{
  int it;

  for (it = 1; it < max_item; it++)
    if ((items[it].flags & ITEM_IN_USE) && items[it].link == item)
      return (it);
  return (0);                   /* no-one owns it */
}

void
remove_item (int item)
{
  short old_link = find_owner (item);
  int player;
  //int max_ship = max_player + max_alien + max_shop;

  if (items[item].sort == pod && items[item].reliability >= BASE_UNIT)
    {
      units[items[item].reliability - BASE_UNIT].pay = 0;
      items[item].reliability = 0;
      items[item].collection = 0;
    }
  
  if (old_link)
    {
      items[old_link].link = items[item].link;
    }
  else                          /* maybe it's the first item of a ship */
    {
    }
  for (player = 0; player < MAX_SHIP; player++)
    if (ships[player].ship == item)
      {
        ships[player].ship = items[item].link;
      }
  items[item].link = 0;
}

void
destroy_item (int item)
{
  items[item].flags = 0;        /* not in use */
  remove_item (item);
}

struct ITEM *
lucky_item (struct PLAYER *player, item_sort sort)
{
  struct ITEM *item = items + player->ship;
  int total = 0;
  struct ITEM *first = NULL;

  while (item != items)
    {
      if (item->sort == sort || ((sort == ram) && is_weapon (item->sort)))
        {
          total++;
          if (!first)
            first = item;
        }
      item = items + item->link;
    }
  if (total == 1)
    if ((first->flags & ITEM_DEMO) == 0)
      return (first);
  return (items);
}

void
check_reliability (FILE * fd, struct PLAYER *player)
{
  struct ITEM *item = items + player->ship;

  if (player->star == HOLIDAY || player->star >= MAX_STAR)
    return;

  while (item != items)
    {
      if (item->sort < pod)
        {
          if (item->flags & ITEM_DEMO)
            item->reliability--;
          if ((dice (100) >= item->reliability) &&
              !(item->flags & ITEM_BROKEN))
            {
              fprintf (fd, "<p>%s broke</p>\n", item_string (item));
              item->flags |= ITEM_BROKEN;
              item->reliability--;
            }
          if (item->reliability == 0 || item->reliability > 200)
            {
              printf ("Item %d destroyed\n", item - items);
              fprintf (fd, "<p>%s lost</p>\n", item_string (item));
              destroy_item (item - items);
            }
          else if (item->flags & ITEM_DEMO && item->reliability < 60)
            {
              fprintf (fd,
                       "<p class=\"warning\">You have a severely unreliable demo module (%s), you should probably discard it.</p>\n",
                       item_string (item));
              fprintf (fd,
                       "<p>If your ship is mainly like this, you should probably restart.</p>\n");
            }
        }
      item = items + item->link;
    }
}


void
repair (FILE * fd, struct PLAYER *player, struct ITEM *item)
{
  int penalty = item->efficiency * item->efficiency;
  int skill = effective_skill_level (player, repairers[item->sort]);

  if (item->flags & ITEM_LUCKY)
    penalty = 0;
  if (!owner (player, item - items))
    {
      fprintf (fd, "<li>Can't repair %s - it's gone</li>\n", item_string (item));
      return;
    }
  if (dice (100) <
      skill + item->reliability - penalty)
    {
      item->flags &= (~ITEM_BROKEN);
      fprintf (fd, "<li>%s officer fixed %s</li>\n",
               skill_names[repairers[item->sort]], item_string (item));
      player->skills[repairers[item->sort]] |=
        skill_bit (repair_skill, item->sort);
    }
  else
    fprintf (fd, "<li>%s officer failed to fix %s</li>\n",
             skill_names[repairers[item->sort]], item_string (item));
}

void
maintain (FILE * fd, struct PLAYER *player, struct ITEM *item)
{
  int increase = effective_skill_level (player, repairers[item->sort]);

  if ((item->flags & ITEM_LUCKY) == 0)
    increase -= item->efficiency * item->efficiency;
  if (!owner (player, item - items))
    {
      fprintf (fd, "<li>Can't maintain %s - it's gone</li>\n", item_string (item));
      return;
    }
  if (increase > 0)
    item->reliability += increase;
  else
    return;
  
  if (item->reliability > 99)
    item->reliability = 99;
  player->skills[repairers[item->sort]] |=
    skill_bit (maintain_skill, item->sort);
  fprintf (fd, "<li>%s officer maintained %s up to %d%%</li>\n",
           skill_names[repairers[item->sort]], item_string (item),
           item->reliability);
}


int
more_reliable (const void *p1, const void *p2)
{
  const struct ITEM *it1 = p1;
  const struct ITEM *it2 = p2;
  
  return it1->reliability - it2->reliability;
}


void
priority (FILE * fd, struct PLAYER *player, skill_sort officer)
{
  int num_items = 0;
  int i = 0;
  int skill = effective_skill_level (player, officer);
  struct ITEM *it;
  struct ITEM **array;

  if (player->magic_flags & (FLAG_SUPER_ENGINEERING << officer))
    {
      for (  it = items + player->ship; it != items ; it = items + it->link)
        {
          if (repairers[it->sort] == officer)
            {
              it->flags &= (~ITEM_BROKEN);
              it->reliability = 99 - rand_exp(10);
            }
        }      
      fprintf (fd, "<li>Your %s crew goes crazy, tearing modules apart, and putting them back together, but they manage to \"fix\" them all.</li>\n",
               skill_names[officer]);
      return;
    }
  for (  it = items + player->ship; it != items ; it = items + it->link)
    {
      if (repairers[it->sort] == officer && it->reliability < skill)
        {
          if (it->sort <= fighter)
            {
              num_items++;
            }
        }
    }
  array = malloc (num_items * sizeof (struct ITEM *));
  for (  it = items + player->ship; it != items ; it = items + it->link)
    {
      if (repairers[it->sort] == officer && it->reliability < skill)
        {
          if (it->sort <= fighter)
            {
              array[i++] = it;
            }
        }
    }
  qsort (array, num_items, sizeof (struct ITEM *), more_reliable);
  for (i = 0 ; i < min(num_items, skill/4) ; i++)
    {
      int gain_max = skill - array[i]->reliability;
      it = array[i];
      if (gain_max > 0)
        {
          int gain = skill - it->efficiency * it->efficiency;
          it->reliability += max(0, min (gain, gain_max));
          fprintf (fd,
                   "<li>%s crew's priority maintenance improves %s up to %d%%</li>\n",
                   skill_names[repairers[it->sort]], item_string (it),
                   it->reliability);
        }
      else
        {
        fprintf (fd,
                   "<li>%s crew reports priority maintenance not possible on %s</li>\n",
                   skill_names[repairers[it->sort]], item_string (it));
        }
    }
  free (array);
}

  

const char *
item_string (struct ITEM *item)
{
  static char buffer[256];

  if (item->sort == evil_artifact)
    {
      sprintf (buffer, "%s %s", uint32_name ((int) item),
               item->flags & ITEM_BROKEN ? "(U)" : "");
    }
  else if (item->sort == artifact)
    {
      sprintf (buffer, "%s %s", uint32_name (item->magic),
               item->flags & ITEM_BROKEN ? "(U)" : "");
    }
  else
    sprintf (buffer, "%s-%d%c %s",
             item_names[item->sort],
             item - items,
             item->flags & ITEM_DEMO ? 'D' : ' ',
             item->flags & ITEM_BROKEN ? "(U)" : "");
  return (buffer);
}

int 
transfer_item (int item, struct PLAYER *new_owner)
{
  int curse, bad_bits;

  if (item == NO_ITEM)
    return new_owner->ship;

  bad_bits = items[item].magic & 0xff0000;      /* blessings */
  bad_bits |= (items[item].magic & 0xff00) << 8;        /* curses */

  if (bad_bits != 0xff0000)
    {
      do
        curse = dice (8);
      while (bad_bits & (0x10000 << curse));

      if (items[item].sort == artifact) /* curse on transfer */
        items[item].magic |= 0x100 << curse;
    }
  items[item].flags &= ITEM_SAVE_FLAGS;
  remove_item (item);           /* remove from old */
  return add_item (new_owner, item);  /* add to new */
}

void
transmute_items (struct PLAYER *ship, item_sort from, item_sort to)
{
  struct ITEM *item = items + ship->ship;
  struct ITEM *next;

  while (item != items)
    {
      next = items + item->link;
      if (item->sort == from)
        {
          int i = item - items;
          remove_item(i);
          item->sort = to;
          ship->ship = add_item(ship,i);
        }
      item = next;
    }
}

void
generate_ship (struct PLAYER *ship, int tech, int extras, int demo)
{
  item_sort sort;
  int new;

  if (tech < 1)
    tech = 1;
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
  if (extras == 0)
    ship->ship = add_item (ship, new_item (laser, tech, 95, 2, demo));
  sort = extras;
  while (sort--)
    {
      ship->ship =
        add_item (ship, new_item (dice (7), tech, 95, 2, demo));
    }
  sort = extras;
  while (sort--)
    {
      ship->ship =
        add_item (ship, new_item (ram + dice (7), tech, 95, 2, demo));
    }
  ship->ship =
    add_item (ship, new_item (pod, tech, 95, 2, demo));
}

int
weighted_tech ()
{
  int rand_int;

  rand_int = dice (10) + 1;
  switch (rand_int)
    {
    case 1:
    case 2:
    case 3:
      return (1);
    case 4:
    case 5:
    case 6:
    case 7:
      return (2);
    case 8:
    case 9:
      return (3);
    case 10:
      return (4);
    default:
      return (1);
    }
}

void
generate_shop (struct PLAYER *ship)
{
  item_sort sort;
  int new, extras;

  new = new_item (warp_drive, weighted_tech(), 60 + dice(36), 2, FALSE);
  if (new < 0)
    {
      ship->ship = 0;
      return;
    }
  ship->ship = new;
  for (sort = warp_drive; sort <= shield; sort++)
    {
      ship->ship = add_item (ship, new_item (sort, weighted_tech(), 60 + dice(36), 2, FALSE));
    }
  ship->ship = add_item (ship, new_item (pod, weighted_tech(), 95, 2, FALSE));
  if (!dice(2))
    ship->ship = add_item (ship, new_item (life_support, weighted_tech(), 60 + dice(36), 2, FALSE));
  else
    ship->ship = add_item (ship, new_item (sensor, weighted_tech(), 60 + dice(36), 2, FALSE));
  extras = dice(6) + 1;
  while (extras--)
    {
      // include pods as a possible extra in the non-weapon category
      sort = dice(8);
      if (sort <= shield)
        ship->ship = add_item (ship, new_item (sort, weighted_tech(), 60 + dice(36), 2, FALSE));
      else
        ship->ship = add_item (ship, new_item (pod, weighted_tech(), 95, 2, FALSE));
    }
  extras = dice(6) + 1;
  while (extras--)
    {
      ship->ship =
        add_item (ship, new_item (ram + dice (7), weighted_tech(), 60 + dice(36), 2, FALSE));
    }
}

int
is_weapon (item_sort sort)
{
  return (sort >= ram && sort <= fighter);
}

int
mass (struct PLAYER *player)
{
  struct ITEM *item = items + player->ship;
  int total = 0;

  if (item == items)
    return (0);
  do
    {
      if (item->sort != artifact)
        total++;
      if (item->sort == pod)    /* add weight of cargo */
        total += item->collection;
      item = items + item->link;
    }
  while (item != items);
  return (total);
}

int
owner (struct PLAYER *player, int item)
{
  struct ITEM *it = items + player->ship;

  while (it != items)
    {
      if (it == items + item)
        return (TRUE);
      it = items + it->link;
    }
  return (FALSE);
}


int
factor (item_sort sort, struct PLAYER *player)
{
  struct ITEM *item = items + player->ship;
  int result = 0, total = 0, new_result;

  if (item == items)
    return (0);
  /* based on fraction of ship made of the right items */
  do
    {
      if (item->sort != artifact)
        total++;
      if (item->sort == pod)    /* add weight of cargo */
        total += item->collection;
      if (item->sort == sort || (sort == ram && is_weapon (item->sort)))
        if (!(item->flags & ITEM_BROKEN))
          result += item->efficiency;
      item = items + item->link;
    }
  while (item != items);
  if (total == 0)
    total++;                    /* doesn't matter anyway if 0 */
  /* modified up by relevant skill */
  new_result = total * effective_skill_level (player, repairers[sort]);
  if (new_result > (result * 200))
    new_result = (result * 200);
  result = (new_result + (result * 100)) / total;
  result = blessing_mod (player, result, sort);
  result = minister_mod (player, result, sort);
  return (result);
}


void
destroy_ship (struct PLAYER *ship)
{
  struct ITEM *item, *next;
  int ring;

  for (ring = 0; ring < MAX_RING; ring++)
    if (ship->rings_held & (1 << ring))
      init_ring (ring);
  item = items + ship->ship;
  while (item != items)
    {
      next = item->link + items;
      destroy_item (item - items);
      item = next;
    }
  ship->ship = 0;
}

