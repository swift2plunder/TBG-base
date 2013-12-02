#include "cargo.h"
#include "globals.h"
#include <stdio.h>
#include "defs.h"
#include "items.h"
#include "rand.h"
#include "combat.h"

int
any_cargo (struct PLAYER *player, int good)
{
  struct ITEM *item = items + player->ship;

  while ((item->sort != pod) || (item->reliability != good))
    {
      if (item->link == 0)
        return (FALSE);
      item = items + item->link;
    }
  if (item->collection < 1)
    return (FALSE);
  return (TRUE);
}

int
unload_pod (FILE * fd, struct PLAYER *player, int good)
{
  struct ITEM *item = items + player->ship;

  while ((item->sort != pod) || (item->reliability != good))
    {
      if (item->link == 0)
        return (FALSE);
      item = items + item->link;
    }
  if (item->collection < 1)
    return (FALSE);
  item->collection--;
  if (item->collection == 0)
    item->reliability = 0;
  if (item->flags & ITEM_DEMO)
    if (power (player) > dice (100))
      {
        fprintf (fd, "<li>%s wore out while unloading</li>\n",
                 item_string (item));
        destroy_item (item - items);
      }
  return (TRUE);
}

/* returns price if sale worked */
int
sell (FILE * fd, struct PLAYER *player, byte colony)
{
  int i, price;

  if (good_prices[colony] < goods[GOOD_NUMBER (colony)].basic_value)
    return (FALSE);
  if (!unload_pod (fd, player, GOOD_NUMBER (colony)))
    return (FALSE);
  player->energy += price = good_prices[colony];
  good_prices[colony] -= 7 * (goods[GOOD_NUMBER (colony)].basic_value / 20);
  for (i = colony & 31; i < 256; i += 32)
    if (i != colony)
      good_prices[i] += goods[GOOD_NUMBER (colony)].basic_value / 20;
  if (GOOD_NUMBER (colony) >= 28)       /* magic number for contraband */
    {
      if (factor (cloak, player) > dice (100))
        fprintf (fd, "<li>Sold contraband without detection!</li>\n");
      else
        {
          player->enemies |= 1 << RACE_NUMBER (colony);
          fprintf (fd, "<li>Caught selling contraband!</li>\n");
        }
    }
  return (price);
}

int
total_cargo (struct ITEM *ship)
{
  struct ITEM *item = ship;
  int total = 0;

  while (item != items)
    {
      if (item->sort == pod)
        total += item->collection;
      item = items + item->link;
    }
  return (total);
}

void
shuffle_goods (struct ITEM *item, int good)
{
  struct ITEM *to, *from, temp;

  from = find_good (item, good);
  to = find_good (item, 0);
  if (!from || !to)
    return;
  if (from->collection > to->efficiency || to->collection > from->efficiency)
    return;
  if (from->reliability >= BASE_UNIT || to->reliability >= BASE_UNIT)
    return;
  temp = *to;
  to->reliability = from->reliability;
  to->collection = from->collection;
  from->reliability = temp.reliability;
  from->collection = temp.collection;
}

/* returns TRUE if ship has any cargo capacity left unloaded */
struct ITEM *
any_room (struct ITEM *item)
{
  while (item != items)
    {
      if (item->sort == pod && item->efficiency > item->collection)
        return (item);
      item = items + item->link;
    }
  return (FALSE);
}

void
swap_goods (struct ITEM *ship, struct ITEM *from)
{
  struct ITEM *to = find_good (ship, 0), temp;

  if (from == to)
    return;
  if (from->collection > to->efficiency || to->collection > from->efficiency)
    return;
  if (from->reliability >= BASE_UNIT || to->reliability >= BASE_UNIT)
    return;
  temp = *to;
  to->reliability = from->reliability;
  to->collection = from->collection;
  from->reliability = temp.reliability;
  from->collection = temp.collection;
}

void
rationalise_cargo (struct ITEM *item, int scale)
{
  struct ITEM *it, temp, *original = item;

  while (item != items)
    {
      it = original;
      while (it != items)
        {
          if (item != it
              && item->sort == pod
              && it->sort == pod
              && item->efficiency > scale
              && item->collection == scale
              && it->collection <= item->efficiency
              && it->efficiency == scale)
            {
              temp = *it;
              it->reliability = item->reliability;
              it->collection = item->collection;
              item->reliability = temp.reliability;
              item->collection = temp.collection;
            }
          if (any_room (item))
            combine_goods (item, any_room (item)->reliability);
          it = items + it->link;
        }
      item = items + item->link;
    }
}

int
any_available (struct ITEM *item, int good)
{
  while (item != items)
    {
      if (item->sort == pod && item->collection == 0)
        return (TRUE);
      if (item->sort == pod && item->reliability == good &&
          item->collection < item->efficiency)
        return (TRUE);
      item = items + item->link;
    }
  return (FALSE);
}

/* returns TRUE if pod could be (and was) loaded with cargo specified */
int
inner_load_pod (struct ITEM *item, int good, int amount, int depth)
{
  struct ITEM *it = item;
  /* for pods, pervert efficiency to be capacity, reliability to be
     sort of cargo, and collection to be amount carried */

  if (--depth == 0)
    return (FALSE);
  for (;; it = items + it->link)
    {
      if (it == items)
        {
          /* this is the exit point, can't buy cargo, but
             there are still possibilities */
          if (any_room (item) && find_good (item, good))
            {
              shuffle_goods (item, good);
              if (any_available (item, good))
                return (inner_load_pod (item, good, amount, depth));
            }
          /* it's worse, but may still be fixable */
          rationalise_cargo (item, 1);
          if (any_room (item))
            {
              struct ITEM *it = item;
              while (it->sort != pod)
                it = items + it->link;
              while (it != items && it->sort == pod)
                {
                  swap_goods (item, it);
                  if (any_room (item)
                      && find_good (item, any_room (item)->reliability))
                    {
                      combine_goods (item, any_room (item)->reliability);
                      if (any_available (item, good))
                        return (inner_load_pod (item, good, amount, depth));
                    }
                  it = items + it->link;
                }
            }
          return (inner_load_pod (item, good, amount, depth));
        }
      if (it->sort != pod)
        continue;               /* not a pod */
      if (it->reliability == 0) /* empty */
        it->collection = 0;     /* conceal bugs */
      if (it->efficiency < it->collection + amount)
        continue;               /* it's full */
      if ((it->reliability != good) && (it->collection))
        continue;               /* it contains another sort of good */
      /* must be ok now */
      it->reliability = good;
      if (good)
        it->collection += amount;
      return (TRUE);
    }
}

/* returns TRUE if pod could be (and was) loaded with cargo specified */
int
load_pod (struct ITEM *item, int good, int amount)
{
  int old_total, new_total, status;

  old_total = total_cargo (item);
  status = inner_load_pod (item, good, amount, 10);
  new_total = total_cargo (item);
  if (status && good)
    new_total -= amount;
  if (old_total != new_total)
    printf ("Bad load of %s - %d != %d (item %d)\n",
            goods[good].name, old_total, new_total, item - items);
  return (status);
}

void
combine_goods (struct ITEM *item, int good)
{
  struct ITEM *from = item, *to = item, *next = NULL;

  if (good == 0)
    return;
  do
    {
      if (next)
        to = next;
      to = find_good (to, good);
      next = items + to->link;
    }
  while (to->collection == to->efficiency);
  next = NULL;
  do
    {
      if (next)
        from = next;
      from = find_good (from, good);
      if (!from)
        break;
      next = items + from->link;
    }
  while (from == to || (from->collection + to->collection > to->efficiency));
  if ((!from) || (!to))
    return;
  to->collection += from->collection;
  from->collection = 0;
  from->reliability = 0;
}

void
init_prices ()
{
  int i;

  for (i = 0; i < 256; i++)
    {
      good_prices[i] = goods[GOOD_NUMBER (i)].basic_value * 3;
    }
}

struct ITEM *
find_good (struct ITEM *item, int good)
{
  while (item != items)
    {
      if (item->sort == pod)
        {
          if (good && item->reliability == good)
            return (item);
          if (good == 0 && item->collection < item->efficiency)
            return (item);
        }
      item = items + item->link;
    }
  return (0);
}


