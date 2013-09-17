#include "defs.h"
#include "tbg.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "globals.h"
#include "dybuk.h"
#include "items.h"
#include "util.h"
#include "locations.h"
#include "terminals.h"
#include "rand.h"
#include "combat.h"
#include "tbg-big.h"
#include "skill.h"
#include "orders.h"

static const char *evil_things[] =
  {
    "bangpedo",
    "logipedo",
    "biopedo",
    "evil zapper"
  };


void
check_evil (FILE *fd, struct PLAYER *p)
{
  int it;
  skill_sort sk;

  if (p->star >= MAX_STAR)
    return;

  if (p == dybuk)
    {
      fprintf (fd, "<P>The voices inside your head want you to:<BR>\n");
      for (sk = engineering ; sk <= weaponry; sk++)
        {
          generate_evil_voices (fd, p, sk);
        }
      if (p->evil < 175)
        fprintf (fd, "And they're getting pretty loud%s<BR>\n",
                 (p->evil < 150) ?
                 ", and you don't think you can resist them much longer"
                 : "");
      return;
    }
  it = p->ship;
  while (it)
    {
      if (items[it].sort == evil_artifact)
          break;
      it = items[it].link;
    }
  if (it)
    {
      int cost = rand_exp(2);
      if (p->popcorn >= cost)
        {
          p->popcorn -= cost;
        }
      else
        {
          fprintf (fd,
                   "<BR>Containment on your evil zapper failed due to insufficient energy!");
          for (sk = engineering ; sk <= medical; sk++)
            {
              int bombs = (items[it].magic & (0x03ff << (10 * sk))) >> (10*sk);
              int n = isqrt(bombs);
              switch (sk)
                {
                case engineering:
                  bangpedo (fd, p, p, n);
                  break;
                case science:
                  logipedo (fd, p, p, n);
                  break;
                case medical:
                  biopedo (fd, p, p, n);
                  break;
                case weaponry:
                default:
                  break;
                }
            }
          items[it].magic &= 0xc0000000;
        }
    }
}

void
update_popcorn ()
{
  int loc;
  int found = 0;
  for (loc = 0 ; loc < MAX_LOCATION ; loc++)
    {
      if (locations[loc].star == popcorn.star
          && (locations[loc].sort == deep_space
              || locations[loc].sort == near_space))
        {
          found = 1;
          break;
        }
    }
  if (! found)
    {
      reset_popcorn ();
      return;
    }
  if (dice (100) < (turn - popcorn.last_released)/25 && ! dybuk)
    {
      fprintf (times, "<HR>Evil is once again unchained!\n");
      assign_dybuk ();
      reset_popcorn ();
      return;
    }
  if (dice (100) < (turn - popcorn.last_collected)/10)
    {
      reset_popcorn();
      return;
    }
  if ((turn - popcorn.last_collected) > 10)
    {
      int factor;

      factor = max (100 - rand_exp ((turn - popcorn.last_collected)/10), 0);
      popcorn.impulse_limit *= factor;
      popcorn.impulse_limit /= 100;
      
      factor = max (100 - rand_exp ((turn - popcorn.last_collected)/10), 0);
      popcorn.sensor_limit *= factor;
      popcorn.sensor_limit /= 100;
      
      factor = max (100 - rand_exp ((turn - popcorn.last_collected)/10), 0);
      popcorn.shield_limit *= factor;
      popcorn.shield_limit /= 100;
    }
  popcorn.reward += rand_exp ((turn - popcorn.last_released)/5);
}


void
make_evilpedos (FILE *fd, struct PLAYER *player, int num, skill_sort officer)
{
  int it;
  int zapper = 0;

  if (num <= 0)
    return;
  it = player->ship;
  while (it)
    {
      if (items[it].sort == evil_artifact)
        {
          zapper = it;
          break;
        }
      it = items[it].link;
    }
  if (officer == weaponry)
    {
      if (zapper)
        {
          fprintf (fd, "You already have an evil zapper<br>\n");
          return;
        }
      else
        {
          int n = count_modules (items + player->ship, (module_type (artifact)));
          int m = dice (n);
          if (!n || player->popcorn < 5)
            {
              fprintf (fd, "Your weaponry officer can't find what he needs to make an evil zapper\n<br>");
              return;
            }
          m = get_nth_module (items + player->ship, module_type(artifact), m);
          remove_item (m);
          fprintf (fd,
                   "Your weaponry officer manages to convert %s into an evil zapper\n<br>",
                   item_string (items + m));
          player->popcorn -= 5;
          player->evil += 20;
          items[m].sort = evil_artifact;
          items[m].magic = dice(4)<<30;
          items[m].flags &= ~ITEM_BROKEN;
          add_item (player, m);
        }
      return;
    }
  else
    {
      if (player->popcorn < num || player->torps < num)
        {
          fprintf (fd,
                   "Your %s officer can't find enough stuff to make %d %s%s<br>\n",
                   skill_names[officer],
                   num,
                   evil_things[officer],
                   (num > 1) ? "s" : "");
          return;
        }
      player->popcorn -= num;
      player->torps -= num;
      player->evil += 2*num;
      fprintf (fd,
               "Your %s officer makes %d %s%s<br>\n",
               skill_names[officer],
               num,
               evil_things[officer],
               (num > 1) ? "s" : "");

      if (! zapper)
        {
          int n = isqrt (num);
          fprintf (fd, "You don't have an evil zapper to hold your %s%s<br>\n",
                   evil_things[officer], (num > 1) ? "s" : "");
          switch (officer)
            {
            case engineering:
              bangpedo (fd, player, player, n);
              break;
            case science:
              logipedo (fd, player, player, n);
              break;
            case medical:
              biopedo (fd, player, player, n);
              break;
            case weaponry:
            default:
              break;
            }
          return;
        }
      else
        {
          int n = (items[zapper].magic >> (officer * 10)) & 0x03ff;
          n += num;
          if (n > 0x03ff)
            n = 0x03ff;
          items[zapper].magic &= ~(0x03ff << (officer * 10));
          items[zapper].magic |= ((n & 0x03ff) << (officer * 10));
        }
    }
}


void
reset_popcorn ()
{
  do
    popcorn.star = dice (MAX_STAR);
  while (star_has_loc (popcorn.star, deep_space) == NO_LOCATION);
  set_bit (evil_stars, popcorn.star);
  popcorn.impulse_limit = dice (10);
  popcorn.sensor_limit = dice (10);
  popcorn.shield_limit = dice (10);
  popcorn.last_collected = turn;
  printf ("Popcorn moved to %d: %s\n", popcorn.star, star_names[popcorn.star]);
}


void
logipedo (FILE * fd, struct PLAYER *attacker, struct PLAYER *enemy, int damage)
{
  int target;

  if (damage <= 0)
    return;

  if (attacker->star < MAX_STAR)
    fprintf (times, "<HR>Logipedos used at %s!\n", star_names[attacker->star]);
  fprintf (fd, "<BR>%s fires %d logipedos!\n",
           name_string (attacker->name),
           damage * damage);
  target = star_has_loc (attacker->star, terminal);
  if (target != NO_LOCATION)
    {
      purge_accounts (attacker);
      fprintf (fd, "<BR>All other access to terminal lost\n");
    }
  if (enemy)
    {
      if (enemy->artifacts & PROTECT_SHIP_BIT)
        {
          fprintf (fd, "<BR>%s protects the computers of %s\n",
                   god_names[engineering], name_string (enemy->name));
        }
      else
        {
          int i;
          uint32 mask;
          for (i = 0 ; i < 32 ; i++)
            {
              if (dice(100) >= 5 * damage)
                mask |= 1<<i;
            }
          enemy->experience[science] &= mask;
          if (mask)
            fprintf (fd, "<BR>Access to terminals by %s lost\n",
                     name_string (enemy->name));
        }
    }
}

void
biopedo (FILE * fd, struct PLAYER *attacker, struct PLAYER *enemy, int damage)
{
  int target;

  if (damage <= 0)
    return;

  if (attacker->star < MAX_STAR)
  fprintf (times, "<HR>Biopedos used at %s!\n", star_names[attacker->star]);
  fprintf (fd, "<BR>%s fires %d biopedos!\n",
           name_string (attacker->name),
           damage * damage);
  target = star_has_loc (attacker->star, homeworld);
  if (target != NO_LOCATION)
    {
      races[target].plague += damage;

      if (races[target].plague > 99)
        races[target].plague = 99;

      fprintf (fd, "<BR>Plague explosion on %s homeworld\n",
               races[target].name);
    }
  if (enemy)
    {
      if (enemy->artifacts & PROTECT_CREW_BIT)
        {
          fprintf (fd, "<BR>%s protects the crew of %s\n",
                   god_names[medical], name_string (enemy->name));
        }
      else
        {
          enemy->health -= 15*damage;
          fprintf (fd, "<BR>Severe health loss to %s\n",
                   name_string (enemy->name));
        }
    }
}

void
bangpedo (FILE * fd, struct PLAYER *attacker, struct PLAYER *enemy, int damage)
{
  struct ITEM *item;
  int any = 0;

  if (damage <= 0)
    return;

  if (attacker->star < MAX_STAR)
    fprintf (times, "<HR>Bangpedos used at %s!\n", star_names[attacker->star]);
  fprintf (fd, "<BR>%s fires %d bangpedos!\n",
           name_string (attacker->name),
           damage*damage);
  if (enemy)
    {
      if (enemy->artifacts & PROTECT_SHIP_BIT)
        {
          fprintf (fd, "<BR>%s protects the ship of %s\n",
                   god_names[engineering], name_string (enemy->name));
        }
      else
        {
          item = items + enemy->ship;
          while (item != items)
            {
              if (item->sort < pod && item->efficiency > 1)
                {
                  item->reliability -= damage;
                  any++;
                }
              item = items + item->link;
            }
          if (any)
            fprintf (fd, "<BR>Ship damage to %s\n", name_string (enemy->name));
        }
    }
}

int
is_evil (struct PLAYER *player)
{
  struct ITEM *item;

  if (!player)
    return (FALSE);

  item = items + player->ship;
  while (item != items)
    {
      if (item->sort == evil_artifact)
        return (TRUE);
      item = items + item->link;
    }
  return (FALSE);
}

void
banish_evil (FILE *fd, struct PLAYER *evil)
{
  int item;
  fprintf (fd, "<BR>%s intervenes to banish the Evil from %s!\n",
           god_names[weaponry], evil->name);

  item = evil->ship;
  while (item)
    {
      if (items[item].sort == evil_artifact)
        remove_item(item);
      item = items[item].link;
    }

  if (evil == dybuk)
    {
      dybuk = 0;
      reset_popcorn ();
      fprintf (times,
               "<HR><FONT COLOR=\"GREEN\">Evil has been banished!<BR>But beware the greed of mortals, lest it return</FONT>\n");
      popcorn.reward = dice(10);
      evil->evil = 100;
    }
}

void
generate_evil_options (FILE * fd, struct PLAYER *player, skill_sort officer)
{
  int it;
  it = player->ship;
  while (it)
    {
      if (items[it].sort == evil_artifact)
        break;
      it = items[it].link;
    }
  if (officer == weaponry)
    {
      if (player->popcorn >= 5 && it)
        fprintf (fd, "<OPTION VALUE=E1>Activate Evil Zapper\n");
      if (! it && player->popcorn > 5)
        fprintf (fd, "<OPTION VALUE=E-1>Make Evil Zapper\n");
    }
  else
    {
      int n;
      if (it)
        {
          int bombs = (items[it].magic & (0x03ff << (officer * 10)))
            >> (officer * 10);
          if (bombs)
            {
              for (n = 1 ; n * n < bombs ; n++)
                {
                  fprintf (fd, "<OPTION VALUE=E%d>Fire %d %s%s\n",
                           n*n, n*n, evil_things[officer], (n > 1) ? "s" : "");
                }
            }
        }
      for (n = 1 ; n < min (player->popcorn, player->torps) ; n++)
        fprintf (fd, "<OPTION VALUE=E%d>Make %d %s%s\n",
                 -n, n, evil_things[officer], (n > 1) ? "s" : "");
    }
}

/* if fd is null, parses orders for default */
/* returns TRUE if it had something for this officer to do */
int 
generate_evil_voices (FILE *fd, struct PLAYER *player,
                      int sort)
{
  struct PLAYER *victim = pairing (player);
  int mypower = 0;
  double advantage = 0;
  int it = 0;
  int loc;
  int n = 0;

  mypower = (power (player) - BIG_NUMBER);
  if (victim)
    advantage = (mypower - power (victim))
      /(0.5 * (mypower + power (victim)));
  for (it = player->ship ; it ; it = items[it].link)
    {
      if (items[it].sort == evil_artifact)
        break;
    }
  if (!it)
    {
      printf ("Why was generate_evil_voices called on a ship with no zapper?\n");
      return 0;
    }

  if (fd && victim && sort == weaponry && (dice (100) > 200 - victim->evil))
    {
      fprintf (fd, "Attack %s!<BR>\n", name_string (victim->name));
    }
  if (victim && advantage < 1.0) /* we have an opponent, and might
                                     lose */
    {
      int bombs, n;
      switch (sort)
        {
        case engineering:
        case science:
        case medical:
          if ((dice (100) < 200 - victim->evil))
            {
              bombs = (items[it].magic & (0x03ff << (10 * sort))) >> (10*sort);
              n = 1+dice(isqrt(bombs)-1);
              if (fd)
                {
                  fprintf (fd, "Fire %d %s!<BR>\n", n*n, evil_things[sort]);
                  return 1;
                }
              else
                {
                  char buffer[512];
                  sprintf (buffer, "%c=E%d", skill_names[sort][0], n);
                  parse_order (buffer, player - players);
                  return 1;
                }
            }
          break;
        case weaponry:
          if (victim->evil < 200)
            {
              if (fd)
                {
                  fprintf (fd, "Activate your Zapper!<BR>\n");
                  return 1;
                }
              else
                {
                  char buffer[512];
                  sprintf (buffer, "%c=E1", skill_names[sort][0]);
                  parse_order (buffer, player - players);
                  return 1;
                }
            }
          break;
        }
    }

    
  for (loc = 0; loc < MAX_LOCATION; loc++)
    {
      int type = locations[loc].sort;
      if (locations[loc].star != player->star ||
          (!loc_accessible (player, loc)))
        continue;
      
      if (location_types[type].instability_skill == sort)
        n++;
    }
  if (n)
    {
      int type;
      n = dice (n);
      loc = -1;
      do
        {
          loc++;
          type = locations[loc].sort;
          if (locations[loc].star != player->star ||
              (!loc_accessible (player, loc)))
            continue;
      
          if (location_types[type].instability_skill == sort)
            n--;
        } while (n);
      if (fd)
        {
          player->evil -= location_types[type].max_instability/4;
          switch(sort)
            {
            case engineering:
              fprintf (fd, "Destabilize orbit of %s %d\n",
                       location_types[type].name, loc);
              break;
            case science:
              fprintf (fd, "Dephase %s %d\n",
                       location_types[type].name, loc);
              break;
            case medical:
              fprintf (fd, "Poison %s %d\n",
                       location_types[type].name, loc);
              break;
            case weaponry:
              fprintf (fd, "Attack %s %d\n",
                       location_types[type].name, loc);
              break;
            }
          return 1;
        }
      else
        {
          char buffer[512];
          sprintf (buffer, "%c=U%d", skill_names[sort][0], loc);
          parse_order (buffer, player - players);
          return 1;
        }
    }
  return 0;
}

void
assign_dybuk ()
{
  int max_evil, i, it;
  skill_sort sk;

  if (dybuk)
    return;

  dybuk = players + MAX_PLAYER + dice (MAX_ALIEN);
  max_evil = 600;
  for (i = 1 ; i < MAX_PLAYER ; i++)
    {
      if (players[i].evil > max_evil && ! holiday_star (players[i].star))
        {
          dybuk = players + i;
          max_evil = dybuk->evil;
        }
    }

  printf ("Dybuk assigned to %s\n", name_string (dybuk->name));
  for (it = dybuk->ship ; it ; it = items[it].link)
    {
      if (items[it].sort == evil_artifact)
        break;
    }
  if (! it)
    {
      int n = 8;
      for (it = dybuk->ship ; it ; it = items[it].link)
        {
          if (items[it].sort == artifact && dice(n--) == 0)
            {
              items[it].sort = evil_artifact;
              items[it].magic = 0;
              break;
            }
        }
    }
  if (! it)
    {
      it = new_item (evil_artifact, 0, 0, 0, 0);
      add_item (dybuk, it);
    }
  for (sk = engineering ; sk <= medical; sk++)
    {
      int bombs = (items[it].magic & (0x03ff << (10 * sk))) >> (10*sk);
      int extra = rand_exp(max_evil);
      bombs += extra;
      if (bombs > 0x03ff)
        bombs = 0x03ff;
      items[it].magic &= ~(0x03ff << (sk * 10));
      items[it].magic |= ((bombs & 0x03ff) << (sk * 10));
    }
  popcorn.last_released = turn;
  popcorn.reward = 1 + dice(10);
  snprintf (dybuk->banner, sizeof (dybuk->banner), "Dybuk of Evil");
}

  

void
resolve_evil_interaction (FILE * fd, struct PLAYER *attacker,
                          struct PLAYER *defender)
{
  struct ITEM *item;
  int bombs;
  skill_sort skill;

  item = items + attacker->ship;
  while (item->sort != evil_artifact)
    item = items + item->link;

  printf ("Evil interaction, %s v %s\n",
          attacker->name, defender->name);

  for (skill = engineering ; skill <= medical ; skill++)
    {
      int bombs = (item->magic & (0x03ff << (10 * skill))) >> (10 * skill);
      int fired = min (bombs, attacker->evilpedos[skill]);

      if (attacker->alliance != PLAYER_ALLIANCE && bombs)
        {
          fired = min (1 + dice (bombs), 1 + dice (bombs));
        }

      if (fired > 0)
        {
          int damage = isqrt(fired);
          switch (skill)
            {
            case engineering:
              bangpedo (fd, attacker, defender, damage);
              break;
            case science:
              logipedo (fd, attacker, defender, damage);
              break;
            case medical:
              biopedo (fd, attacker, defender, damage);
              break;
            case weaponry:
            default:
              break;
            }
          bombs -= damage*damage;
        }
      item->magic &= ~(0x03ff << (10 * skill));
      item->magic |= (bombs & 0x03ff) << (10 * skill);
    }
  
  if (defender->blessings & BANISH_EVIL_BIT)
    {
      banish_evil (fd, attacker);
    }
}

void
resolve_interaction (struct PLAYER *p1, struct PLAYER *p2)
{
  FILE *fd;
  char battlefile[256];
  char buf2[256];
  int combat = FALSE;
  int p1_range, p2_range;

  if ((p1->restarting == 1) || (p2->restarting == 1))
    return;
  sprintf (battlefile, "%s/results/%d/battle%d_%d", webroot,
           game, session_id, p1 - players);
  sprintf (buf2, "%s/results/%d/battle%d_%d", webroot,
           game, session_id, p2 - players);
  printf ("Battle between %s (%d)", name_string (p1->name), p1 - players);
  printf (" and %s (%d)\n", name_string (p2->name), p2 - players);
  printf("Battle file for %s: %s\n", name_string (p1->name), battlefile);
  printf("Battle file for %s: %s\n", name_string (p2->name), buf2);
  
  fd = fopen (battlefile, "w");
  if (!fd)
    {
      printf ("Can't create battle file %s (%d)\n", battlefile, errno);
      exit (1);
    }
  fprintf (fd, "<H2>%s meets \n", name_string (p1->name));
  fprintf (fd, "%s at %s</H2>\n",
           name_string (p2->name), star_names[p1->star]);

  if (is_evil (p1) && p1->evilpedos[3] && p2 != dybuk) 
    {
      resolve_evil_interaction (fd, p1, p2);
    }

  if (is_evil (p2) && p2->evilpedos[3] && p1 != dybuk)
    {
      resolve_evil_interaction (fd, p2, p1);
    }

  if ((p1 == dybuk && p1->evilpedos[3])
      || (p2 == dybuk && p2->evilpedos[3]))
    {
      fprintf (fd, "<P>%s uses EvilTech, conventional modules are useless!\n",
               dybuk->name);
      fprintf (fd, "<BR>%s withdraws from combat unhurt!\n", dybuk->name);
      fclose (fd);
      link (battlefile, buf2);
      return;
    }

  /* aliens try not to fight dybuk */ 
  if (p1 == dybuk && p2->alliance != PLAYER_ALLIANCE)
    {
      p2->strategy.dip_option = flee;
    }

  if (p2 == dybuk && p1->alliance != PLAYER_ALLIANCE)
    {
      p2->strategy.dip_option = flee;
    }

  /* panic abort combat if peace breaks out in the magic phase */
  if (p1->strategy.dip_option == always_attack &&
      p1->alliance >= 0 && !(p2->enemies & (1 << p1->alliance)))
    {
      printf ("Aborting battle of %s with %s\n", p1->name, p2->name);
      p1->strategy.dip_option = flee;
    }
  if (p2->strategy.dip_option == always_attack &&
      p2->alliance >= 0 && !(p1->enemies & (1 << p2->alliance)))
    {
      printf ("Aborting battle of %s with %s\n", p2->name, p1->name);
      p2->strategy.dip_option = flee;
    }

  if (p1->strategy.demand == 0)
    {
      switch (p1->strategy.dip_option)
        {
        case attack_if_defied:
          p1->strategy.dip_option = always_attack;
          break;
        case make_demands:
          p1->strategy.dip_option = flee;
          break;
        case flee:
        case no_attack:
        case always_attack:
          break;
        }
    }
  if (p2->strategy.demand == 0)
    {
      switch (p2->strategy.dip_option)
        {
        case attack_if_defied:
          p2->strategy.dip_option = always_attack;
          break;
        case make_demands:
          p2->strategy.dip_option = flee;
          break;
        case flee:
        case no_attack:
        case always_attack:
          break;
        }
    }

  if (p1->strategy.dip_option == make_demands ||
      p1->strategy.dip_option == attack_if_defied)
    {
      fprintf (fd, "<BR>%s demands %s\n",
               name_string (p1->name),
               item_string (items + p1->strategy.demand));
      if (items[p1->strategy.demand].flags & ITEM_OFFERED)
        {
          printf ("%s gives %s a gift\n", p2->name, p1->name);
          /* OK, victim hands over tribute */
          p1->ship = transfer_item (p1->strategy.demand, p1);
          p1->strategy.dip_option = no_attack;
          fprintf (fd, " and %s offers it\n", name_string (p2->name));
        }
      else
        {
          if (p1->strategy.dip_option == make_demands)
            p1->strategy.dip_option = no_attack;
          else
            p1->strategy.dip_option = always_attack;
          fprintf (fd, " but %s refuses to give it\n",
                   name_string (p2->name));
        }
    }

  if (p2->strategy.dip_option == make_demands ||
      p2->strategy.dip_option == attack_if_defied)
    {
      fprintf (fd, "<BR>%s demands %s\n",
               name_string (p2->name),
               item_string (items + p2->strategy.demand));
      if (items[p2->strategy.demand].flags & ITEM_OFFERED)
        {
          printf ("%s gives %s a gift\n", p1->name, p2->name);
          /* OK, victim hands over tribute */
          p2->ship = transfer_item (p2->strategy.demand, p2);
          p2->strategy.dip_option = no_attack;
          fprintf (fd, " and %s offers it\n", name_string (p1->name));
        }
      else
        {
          if (p2->strategy.dip_option == make_demands)
            p2->strategy.dip_option = no_attack;
          else
            p2->strategy.dip_option = always_attack;
          fprintf (fd, " but %s refuses to give it\n",
                   name_string (p1->name));
        }
    }

  p1_range = min (p1->strategy.ideal_range, detection_range (p1, p2));
  p2_range = min (p2->strategy.ideal_range, detection_range (p2, p1));

  if (p1->strategy.dip_option == always_attack &&
      p2->strategy.dip_option == always_attack)
    if (p1_range >= p2_range)
      combat = resolve_combat (fd, p1, p2);
    else
      combat = resolve_combat (fd, p2, p1);
  else if (p1->strategy.dip_option == always_attack)
    combat = resolve_combat (fd, p1, p2);
  else if (p2->strategy.dip_option == always_attack)
    combat = resolve_combat (fd, p2, p1);
  fclose (fd);
  link (battlefile, buf2);
  if (combat && is_player (p1) && is_player (p2))
    {
      char buf3[256];
      snprintf (buf3, 256, "%s/battles/%s_%s_%d.html",
                webroot, p1->name, p2->name, turn);
      link (battlefile,buf3);
    }
}



void
harvest_popcorn (FILE * fd, struct PLAYER *player)
{
  int failed = FALSE;
  int best;
  int excess;

  best = excess = factor (impulse_drive, player) - popcorn.impulse_limit;
  if (excess < 0)
    {
      fprintf (fd, "<P><EM>Impulse engines too weak to catch popcorn</EM>\n");
      failed = TRUE;
    }
  excess = factor (sensor, player) - popcorn.sensor_limit;
  best = min (best, excess);
  if (excess < 0)
    {
      fprintf (fd, "<P><EM>Sensors too weak to detect popcorn</EM>\n");
      failed = TRUE;
    }
  excess = factor (shield, player) - popcorn.shield_limit;
  best = min (best, excess);
  if (excess < 0)
    {
      fprintf (fd, "<P><EM>Shields too weak to approach popcorn</EM>\n");
      failed = TRUE;
    }
  if (failed)
    return;
  excess = best;
  best = excess > 0 ? max (rand_exp (excess), 1) : 0;
  fprintf (fd, "<P>Collected %d popcorn\n", best);
  player->popcorn += best;
  popcorn.impulse_limit += rand_exp (best);
  popcorn.sensor_limit += rand_exp (best);
  popcorn.shield_limit += rand_exp (best);
  popcorn.last_collected = turn;
}


void
do_hiding_damage (FILE * fd, struct PLAYER *player)
{
  struct ITEM *item = items + player->ship;
  int risk = risk_level (player, abs (player->hide_hunt));

  if (risk)
    printf ("Hide hunt for %s, risk %d\n", player->name, risk);
  while (item != items)
    {
      if (item->sort < pod)
        {
          if ((dice (100) < risk) && !(item->flags & ITEM_BROKEN))
            {
              fprintf (fd, "<P><EM>%s broke</EM>\n", item_string (item));
              item->flags |= ITEM_BROKEN;
              item->reliability--;
            }
          if (item->reliability == 0 || item->reliability > 200)
            {
              printf ("Item %d destroyed\n", item - items);
              fprintf (fd, "<P><EM>%s lost</EM>\n", item_string (item));
              destroy_item (item - items);
            }
        }
      item = items + item->link;
    }
}


struct
POPCORN_BUYER
{
  int id, num, price, sales;
};



int less_expensive (const void *v1, const void *v2)
{
  const struct POPCORN_BUYER *p1 = v1;
  const struct POPCORN_BUYER *p2 = v2;

  return p2->price - p1->price;
}

  

void
do_popcorn_auction ()
{
  int sales = 8;
  int buyers = 0;
  int popcorn_price;
  int entries;
  int i;
  int n = 0;
  FILE *fd = 0;
  struct POPCORN_BUYER *popcorn_buyers;


  for (i = 1 ; i < MAX_PLAYER ; i++)
    {
      if (players[i].popcorn_price * players[i].popcorn_buy
          > players[i].energy)
        {
          players[i].popcorn_buy =
            players[i].energy / players[i].popcorn_price;
        }
      if ((players[i].popcorn_buy  > 0 && players[i].popcorn_price > 0)
          || players[i].popcorn_sales > 0)
        {
          buyers++;
        }
      if (players[i].popcorn_sales > 0)
        {
          if (players[i].popcorn_sales > players[i].popcorn)
            players[i].popcorn_sales = players[i].popcorn;
          sales += players[i].popcorn_sales;
          //          players[i].popcorn -= players[i].popcorn_sales;
        }
    }
  entries = buyers + sales;
  popcorn_buyers = malloc (entries * sizeof (struct POPCORN_BUYER));
  
  for (i = 1 ; i < MAX_PLAYER ; i++)
    {
      if ((players[i].popcorn_buy  > 0 && players[i].popcorn_price > 0)
          || players[i].popcorn_sales > 0)
        {
          popcorn_buyers[n].id = i;
          popcorn_buyers[n].num = players[i].popcorn_buy;
          popcorn_buyers[n].price = players[i].popcorn_price;
          popcorn_buyers[n].sales = players[i].popcorn_sales;
          n++;
        }
      players[i].popcorn_buy = 0;
    }
  for (i = 0 ; i < sales ; i++)
    {
      popcorn_buyers[n].id = 0;
      popcorn_buyers[n].num = 1;
      popcorn_buyers[n].price = max (10000/(i + 1), 1);
      popcorn_buyers[n].sales = 0;
      n++;
    }
  qsort (popcorn_buyers, entries, sizeof(struct POPCORN_BUYER), less_expensive);
  for (n = 0 ; sales ; n++)
    {
      if (popcorn_buyers[n].num > sales)
        {
          popcorn_buyers[n].num = sales;
        }
      sales -= popcorn_buyers[n].num;
      i = popcorn_buyers[n].id;
      if (i)
        {
          players[i].popcorn_buy = popcorn_buyers[n].num;
          players[i].popcorn += players[i].popcorn_buy;
        }
    }
  popcorn_price = popcorn_buyers[n-1].price;
  for (n = 0 ; n < entries ; n++)
    {
      i = popcorn_buyers[n].id;
      if (i)
        {
          int amount = players[i].popcorn_buy - players[i].popcorn_sales;
          struct PLAYER *player = players + i;
          open_results (&fd, player);

          if (amount == 0)
            fprintf (fd, "<P>Didn't trade any popcorn, going price was$%d\n",
                     popcorn_price);
          else if (amount > 0)
            {
              fprintf (fd, "<P>Bought %d Popcorn for $%d each, total $%d\n",
                       amount, popcorn_price, amount * popcorn_price);
            }
          else
            {
              fprintf (fd, "<P>Sold %d Popcorn for $%d each, total $%d\n",
                       -amount, popcorn_price, -amount * popcorn_price);
            }
          player->popcorn += amount;
          player->energy -= amount * popcorn_price;
          player->popcorn_sales = player->popcorn_buy = 0;
        }
    }
  if (fd)
    fclose(fd);
  free (popcorn_buyers);
}

  

void
buy_sell_popcorn (FILE * fd, struct PLAYER *player)
{
}


void
destabilize (FILE *fd, struct PLAYER *player, int loc)
{
  if (loc > 0)
    {
      int type = locations[loc].sort;
      skill_sort sort = location_types[type].instability_skill;
      int skill  = effective_skill_level (player, sort);
      int kaboom = 0;
      int evil;
      long pop;
      int max_i = location_types[type].max_instability;
      
      if (player->magic_flags & (FLAG_SUPER_ENGINEERING << sort))
        skill *= 2;
  
      if (player == dybuk)
        skill = 129;
  
      locations[loc].instability += skill;

      if (locations[loc].instability > max_i)
        kaboom = 1;
  
      evil = (kaboom ? 2 : 1) * max_i/4;
      skill = (skill > max_i) ? max_i : skill;
      pop = rand_exp ((popcorn.reward * (kaboom ? 2 : 1) * skill * max_i) /
                      (1024.0*128.0*2.0));

      switch(sort)
        {
        case engineering:
          fprintf (fd, "<BR>Your engineering crew destabilizes the orbit of %s%d%s\n",
                   location_types[type].name, loc,
                   (kaboom)? " and it falls into the star." :
                   ", but it still hasn't fallen into the sun.");
          break;
        case science:
          fprintf (fd, "<BR>Your science crew dephases the polarity of %s%d%s\n",
                   location_types[type].name, loc,
                   (kaboom)? " and it collapses into a subspace vacuole."
                   : ", but it hasn't collapsed yet.");
          break;
        case medical:
          fprintf (fd, "<BR>Your medical crew poisons %s%d%s\n",
                   location_types[type].name, loc,
                   (kaboom)? " and everyone dies." :
                   ", killing many, but many are still living.");
          break;
        case weaponry:
          fprintf (fd, "<BR>Your weaponry crew attacks %s%d%s\n",
                   location_types[type].name, loc,
                   (kaboom)? " wiping it out completely."
                   : ", killing many, but many are still living.");
          break;
        }
      fprintf (fd, " and collects %d popcorn in the process.\n", pop);
      player->popcorn += pop;
    }
  else
    {
      skill_sort sort;
      int skill;
      int kaboom = 0;
      int pop;
      switch (loc)
        {
        case -1:
          sort = engineering;
          break;
        case -2:
          sort = science;
          break;
        case -3:
          sort = medical;
          break;
        case -4:
          sort = weaponry;
          break;
        default:
          return;
        }
      skill = effective_skill_level (player, sort);
      if (player->magic_flags & (FLAG_SUPER_ENGINEERING << sort))
        skill *= 2;
  
      if (player == dybuk)
        skill = 129;
      stars[player->star].instability += skill;

      if (stars[player->star].instability > 1024)
        kaboom = 1;
  
      player->evil += (kaboom ? 128 : 64);
      pop = rand_exp ((popcorn.reward * (kaboom ? 2 : 1) * skill) /
                      (128*2));

      switch(sort)
        {
        case engineering:
          fprintf (fd, "<BR>Your engineering crew induces sunspots in the star%s",
                   (kaboom)? " and it goes supernova" : "");
          break;
        case science:
          fprintf (fd, "<BR>Your science crew induces solar flares in the star%s",
                   (kaboom)? " and it goes supernova" : "");
          break;
        case medical:
          fprintf (fd, "<BR>Your medical crew feeds the stellar amoebae%s",
                   (kaboom)? " and the star goes supernova" : "");
          break;
        case weaponry:
          fprintf (fd, "<BR>Your weaponry crew fires into the star%s",
                   (kaboom)? " and it goes supernova" : "");
          break;
        }
      fprintf (fd, ", and collects %d popcorn doing so.\n", pop);
      player->popcorn += pop;
    }
}

void
check_stability ()
{
  int loc;
  int star;
  for (loc = 0 ; loc < MAX_LOCATION ; loc++)
    {
      int type = locations[loc].sort;
      if (dybuk && dybuk->alliance != PLAYER_ALLIANCE
          && locations[loc].star == dybuk->star)
        locations[loc].instability += dice(64);

      if (locations[loc].instability >= location_types[type].max_instability)
        {
          locations[loc].instability = 0;
          relocate(loc);
        }
      else
        {
          locations[loc].instability /= 2;
        }
    }
  for (star = 0 ; star < MAX_STAR ; star ++)
    {
      if (stars[star].instability > 1024)
        {
          stars[star].instability = 0;
          supernova(star);
        }
      else
        {
          stars[star].instability /= 2;
        }
    }
}

  
void
update_evil (FILE * fd, struct PLAYER *player)
{
  skill_sort skill;
  int atonements = 0;
  int evil_minimum = 100;
  int extra_evil;
  for (skill = engineering ; skill <= weaponry ; skill++)
    {
      if (player->rings_held & (1 << skill))
        {
          player->evil += 2;
        }
      if (player->magic_flags & FLAG_ATONE_ENGINEERING << skill)
        {
          atonements += 100;
        }
      else
        {
          if (player->chosen & (1 << skill))
            {
              if (300 + dice(200) < player->evil)
                {
                  fprintf (fd, "<BR>%s is displeased with you, and casts you out from their Chosen\n",
                           god_names[skill]);
                  atonements += 100;
                  player->chosen &= ~(1 << skill);
                }
            }
          else if (enlightened (player, skill))
            {
              if (500 + dice(1000) < player->evil)
                {
                  fprintf (fd, "<BR>%s is very displeased with you, and strips you of your enlightenment\n",
                           god_names[skill]);
                  player->skills[skill] &= ~0x40;
                  atonements += 100;
                }
              else if (500 < player->evil)
                {
                  int n = 0;
                  int f = player->favour[skill];
                  fprintf (fd, "<BR>%s is very displeased with you",
                           god_names[skill]);
                  if (f > 100)
                    n = rand_exp ((f - 100)/10);
                  if (n > 1)
                    {
                      fprintf (fd, "and takes back %d favor", n);
                      player->favour[skill] -= n;
                      atonements += 50 - 50/sqrt(n);
                    }
                  fprintf (fd, ".\n");
                }
            }
        }
      if (player->chosen & (1 << skill))
        evil_minimum = 0;
    }
  if (atonements)
    {
      extra_evil = player->evil - evil_minimum;
      extra_evil *= 400 - atonements;
      extra_evil /= 400;
      player->evil = evil_minimum + extra_evil;
    }
}

  
