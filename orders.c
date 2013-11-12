#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "defs.h"
#include "tbg.h"
#include "bytes.h"
#include "globals.h"
#include "items.h"
#include "util.h"
#include "tbg-big.h"
#include "politics.h"
#include "dybuk.h"
#include "adventures.h"
#include "criminals.h"
#include "crew.h"
#include "locations.h"
#include "cargo.h"
#include "aliens.h"
#include "combat.h"
#include "skill.h"
#include "rand.h"
#include "religion.h"
#include "movement.h"

#define MAX_ORDER 6000

/* FIXME need in a header somewhere */

struct PLAYER * pairing (struct PLAYER *player);
struct PLAYER * sort_ships (int star);

struct ORDER
{
  char command;
  byte player;
  int param1, param2;
};

struct ORDER orders[MAX_ORDER];

/* used internally only */

void parse_combat (char command, char *buffer, int player);

void
parse_order (char *buffer, int player)
{
  char sub_command;
  skill_sort officer;
  static struct ORDER *order = orders;

  if ((order - orders > MAX_ORDER - 500) && (!really_send))
    {
      printf ("Too many orders!\n");
      exit (1);
    }
  if (player > MAX_PLAYER || want_verbose)
    printf ("Player = %d, order is %s\n", player, buffer);
  if (buffer[0] == '#')
    {
      strcpy (players[player].x_from, buffer + 2);
      return;
    }
  if (buffer[0] == 't' && buffer[1] == 'o')
    return;
  if (buffer[2] == ' ')         /* detect c= nA case */
    return;
  order->player = player;
  order->command = *buffer;
  sub_command = buffer[1];
  buffer += 2;                  /* skip command and = */
  switch (order->command)
    {
    case 'E':                   /* engineer */
      officer = engineering;
      break;
    case 'S':                   /* science */
      officer = science;
      break;
    case 'M':                   /* medicine */
      officer = medical;
      break;
    case 'W':                   /* weaponry */
      officer = weaponry;
      break;
    }
  switch (order->command)
    {
    case 'E':                   /* engineer */
    case 'S':                   /* science */
    case 'M':                   /* medicine */
    case 'W':                   /* weaponry */
      if (player == dybuk - players && players[player].evil < 150)
        {
          if (generate_evil_voices (0, players + player, officer))
            {
              players[player].evil += 5;
              break;
            }
        }
      if (*buffer >= '0' && *buffer <= '9')     /* collect */
        {
          order->command = 'c';
          order->param1 = atoi (buffer);
          while (*buffer == ' ')
            buffer++;
          while (*buffer >= '0' && *buffer <= '9')
            buffer++;
          order->param2 = *buffer;
          break;
        }
      order->command = *buffer++;
      switch (order->command)
        {
        case 'a':
        case 'A':               /* adventure */
          order->param1 = atoi (buffer);
          players[player].away_team[officer] = order->param1;
          if (ADVENTURE_SKILL (order->param1) == officer)
            break;
          else
            {
              order->command = 'X';
              return;           /* all done */
            }
        case 'E': /* evil */
          {
            int n;
            n = atoi(buffer);
            if (n > 0)
              {
                players[player].evilpedos[officer] = n;
                order->param1 = 0;
              }
            else
              {
                order->param1 = -n;
                order->param2 = officer;
              }
          }
          break;
        case 'G':               /* bounty hunting */
          order->param1 = atoi (buffer);
          break;
        case 'k':               /* authentication key */
          break;
        case 'K':               /* cure */
          break;
        case 'h':
        case 'H':               /* heal */
          order->param1 = atoi (buffer);
          break;
        case 'i':
        case 'I':               /* interrogate */
          if (officer == weaponry)
            order->param1 = atoi (buffer);
          else
            {
              printf ("Naughty interrogation attempt by %s!\n",
                      players[player].name);
              order->command = '.';
            }
          break;
        case 'l':
        case 'L':               /* long range scan, communing */
          order->param1 = atoi (buffer);
          break;
        case 'm':               /* priority maintenance */
          order->param1 = officer;
          break;
        case 'M':               /* maintain */
          order->param1 = atoi (buffer);
          break;
        case 'r':
        case 'R':               /* repair */
          order->param1 = atoi (buffer);
          break;
        case 't':
        case 'T':               /* train crew */
          order->param1 = atoi (buffer);
          break;
        case 'v':
        case 'V':               /* try password */
          order->param1 = atoi (buffer);
          if (order->param1 <= 3 && officer)
            order->param1 += officer * 4;
          break;
        case 'W':               /* collect wraith ring */
          order->param1 = atoi (buffer);
          break;
        case 'U':               /* unstabilize (destabilize) */
          order->param1 = atoi (buffer);
          break;
        case '.':               /* rest */
          order->param1 = officer;
          players[player].standby |= 1 << officer;
          break;
        default:
          printf ("Funny main option %c for %s\n",
                  *buffer, players[player].name);
          break;
        }
      break;
    case 'b':                   /* scrapping things */
      order->param1 = atoi (buffer);
      break;
    case 'c':                   /* collect resources from location */
      order->param1 = atoi (buffer);
      while (*buffer == ' ')
        buffer++;
      if ((*buffer < '0' || *buffer > '9') && *buffer != '-')
        return;                 /* evil white space trap */
      while ((*buffer >= '0' && *buffer <= '9') || *buffer == '-')
        buffer++;
      order->param2 = *buffer;
      break;
    case 'd':                   /* diplomatic */
      parse_combat (sub_command, buffer + 1, player);
      break;
    case 'D':                   /* disable */
      order->param1 = atoi (buffer);
      break;
    case 'e':                   /* explore (positive) / choose (negative) */
      if ((*buffer < '0' || *buffer > '9') && (*buffer != '-'))
        {
          /* evil browser can send text for no options select */
          order->param1 = 0;
          break;
        }
      order->param1 = atoi (buffer);
      if (order->param1 >= 0)
        break;
      /* else use this handy code to set param2 as well */
      while (*buffer == ' ')
        buffer++;
      if ((*buffer < '0' || *buffer > '9') && *buffer != '-')
        return;                 /* evil white space trap */
      while ((*buffer >= '0' && *buffer <= '9') || *buffer == '-')
        buffer++;
      order->param2 = *buffer;
      break;
    case 'f':                   /* flag/banner */
      if (!dybuk || players + player != dybuk)
        {
          if (strlen (buffer) > 100)
            printf ("Big banner from %s\n", players[player].name);
          buffer[99] = '\0';
          strncpy (players[player].banner_source, buffer, 100);
          remove_html (buffer);
          strncpy (players[player].banner, buffer, 100);
        }
      else
        snprintf (dybuk->banner, sizeof (dybuk->banner), "Dybuk of Chaos");
      break;
    case 'g':                   /* excommunicate/forgive */
      order->param1 = atoi (buffer);
      while (*buffer == ' ')
        buffer++;
      while (*buffer >= '0' && *buffer <= '9')
        buffer++;
      order->param2 = *buffer - 'A';
      break;
    case 'j':                   /* jump to another system */
      if (*buffer && (*buffer < '0' || *buffer > '9') && *buffer != '-'
          && *buffer != ' ')
        order->param1 = decode_starname (buffer, players[player].movement);
      else
        order->param1 = atoi (buffer);
      players[player].movement = order->param1;
      if (order->param1 > BIG_NUMBER)
        {
          if (players[player].star !=
              players[order->param1 - BIG_NUMBER].star)
            {
              printf ("Bad chase by %s!\n", players[player].name);
              order->param1 = players[player].star;
            }
        }
      break;
    case 'l':                   /* alliances */
      order->param1 = atoi (buffer);
      break;
    case 'n':                   /* name */
      break;
    case 'N': /*buy popcorn */
      players[player].popcorn_buy = atoi(buffer);
    case 'O': /*popcorn price */
      players[player].popcorn_price = atoi(buffer);
      break;
    case 'o':                   /* vaguer rumour */
      press (0, buffer, "Anonymous");
      break;
    case 'P':                   /* presidential command */
      if (players[player].star >= MAX_STAR)
        break;
      order->param1 = *buffer++;
      order->param2 = atoi (buffer);
      political_command (players + player, order->param1, order->param2);
      break;
    case 'p':                   /* press */
      press (player, buffer, players[player].address);
      break;
    case 'q':                   /* rumour */
      press (player, buffer, name_string (players[player].name));
      break;
    case 's':
      if (players[player].star >= MAX_STAR)
        break;
      order->param1 = atoi (buffer);
      if (order->param1 >
          players[player].popcorn - players[player].popcorn_sales)
        order->param1 =
          players[player].popcorn - players[player].popcorn_sales;
      players[player].popcorn_sales += order->param1;
      break;
    case 'u':                   /* user preferences */
      order->param1 = atoi (buffer);
      if (order->param1 > 0)
        players[player].preferences += order->param1;
      else
        players[player].bid -= order->param1;
      order->command = 'X';
      return;
    case 'w':                   /* web source */
      if (strstr (buffer, "pbm.com"))
        strcpy (buffer, "http://tbg.asciiking.com/");
      strcpy (players[player].web_source, buffer);
      break;
    case 'x':                   /* magic spell */
      order->param1 = atoi (buffer);
      while (*buffer == ' ')
        buffer++;
      while ((*buffer >= '0' && *buffer <= '9') || *buffer == '-')
        buffer++;
      order->param2 = *buffer - 'A';
      if (order->param1 >= BIG_NUMBER)  /* tracer */
        {
          order->param2 = order->param1 - BIG_NUMBER;
          order->param1 = MAGIC_TRACE_SHIP;
        }
      break;
    case 'y':                   /* new player starts up */
      order->param1 = players[player].restarting = atoi (buffer);
      printf ("%s restarting (%d)\n",
              players[player].name, players[player].restarting);
      if (players[player].restarting == 0)
        players[player].restarting = 1;
      break;
    case 'z':                   /* turn */
      break;
    case 'Z':                   /* game number */
      if (atoi (buffer) != game)
        {
          printf ("Reading orders for wrong game\n");
          exit (1);
        }
      break;
    case 'X':                   /* end of orders */
      break;
    case '\n':                  /* blank lines inserted by tightbeam */
      break;
    case 'K':
      order->command = 'k';
      printf ("Fixing key for %s\n", players[player].name);
      break;
    case 'k':                   /* authentication key */
      break;
    default:
      printf ("Bad command <<%c>>\n", order->command);
      break;
    }
  order++;
  order->command = 'X';
  if (order - orders > MAX_ORDER)
    {
      printf ("Too many orders!\n");
      exit (1);
    }
}

/* returns actual destination, allowing for chasing */
int
resolve_movement (int player)
{
#define PENDING (BIG_NUMBER - 1)

  int target, result, saved_movement;
  int x = 1;

  /* direct to star or go to or from holiday, easy */
  if (players[player].movement <= PENDING)
    {
      if ((players[player].movement >= MAX_PLAYER && x) ||
          factor (warp_drive, players + player) ||
          players[player].star == HOLIDAY || players[player].star >= MAX_STAR
          || players[player].movement == HOLIDAY
          || players[player].movement >= MAX_STAR)
        return (players[player].movement);
      else
        return (players[player].star);
    }
  else                          /* chasing */
    {
      target = players[player].movement - BIG_NUMBER;
      if (players[target].movement > MAX_PLAYER)
        printf ("%s chasing %s to %d\n",
                players[player].name, players[target].name,
                players[target].movement);
      if (players[target].movement == PENDING)  /* loop */
        return (players[player].star);  /* no move */
      saved_movement = players[player].movement;
      /* need to avoid corrupting this for 3+ way chasing */
      players[player].movement = PENDING;
      result = resolve_movement (target);
      if (result == OLYMPUS && players[player].chosen <= OLYMPUS_SEEN)
        result = players[player].star;
      if (result >= MAX_PLAYER
          && !any_gates (players + player, players[player].star,
                         locations[result].parameter))
        result = players[player].star;
      players[player].movement = saved_movement;
      if (result == NOWHERE)
        {
          printf ("Chasing to Oblivion!\n");
          return (players[player].star);
        }
      return (result);
    }
}

void
execute_orders ()
{
  struct ORDER *order;
  struct PLAYER *player, *base, *previous = 0, *dybuk_enemy;
  int loc;
  int star, p, enemy;
  int rank, last_rank = 1;
  FILE *fd = 0;
  int high_bid = 1, high_bidder = -1;
  int high_strength = 0, high_fighter = -1;
  int old_fee = next_fee;
  int any_changes;

  if (dybuk)
    dybuk_enemy = pairing (dybuk);
  else
    dybuk_enemy = 0;
  order = orders;
  while (order->command != 'X')
    {
      player = players + order->player;
      if (player != previous)
        {
          open_results (&fd, player);
          fprintf (fd, "<!-- orders.c:457 -->");
          previous = player;
        }
      if (order->command == 'x' && player->star < MAX_STAR)
        cast_spell (fd, players + order->player,
                    order->param1, order->param2);
      order++;
    }
  consolidate_artifacts ();     /* allow for any magic */
  /* second phase, interactions */
  for (star = -2; star < MAX_STAR + MAX_PLAYER; star++)
    {
      base = sort_ships (star);
      if (star == HOLIDAY || star >= MAX_STAR)
        continue;
      while (base && base->next)
        {
          if (base->alliance == PLAYER_ALLIANCE
              || base->next->alliance == PLAYER_ALLIANCE)
            resolve_interaction (base, base->next);
          base = base->next->next;
        }
    }
  for (p = 1; p < MAX_PLAYER; p++)
    {
      open_results (&fd, players + p);
      fprintf (fd, "<!-- orders.c:483 -->");
      do_hiding_damage (fd, players + p);
      if (players[p].rank > last_rank)
        last_rank = players[p].rank;
    }
  consolidate_artifacts ();     /* allow for any transfers */
  update_power ();

  /* third phase, everything else */
  rank = 1;
  order = orders;
  do
    {
      if (order->command == 'X')
        {
          rank++;
          order = orders;
        }
      player = players + order->player;
      if (player->star >= MAX_STAR && order->command != 'j') // on holiday
        {
          order++;
          continue;
        }
      if (player->rank != rank)
        {
          order++;
          continue;
        }
      if (player->star >= MAX_STAR)
        {
          order++;
          continue;
        }
      if (player != previous)
        {
          open_results (&fd, player);
          fprintf (fd, "<!-- orders.c:519 -->");
          fprintf (fd, "<h2>Actions</h2>\n<ul>\n");
          previous = player;
          do_password_powers (fd, player);
        }
      switch (order->command)
        {
        case 'A':               /* adventure */
          try_adventure (fd, player, order->param1);
          break;
        case 'b':               /* scrapping */
          scrap (fd, player, order->param1);
          break;
        case 'c':               /* collect from location */
          if (order->param1 >= 0)
            collect (fd, player, order->param1, order->param2);
          else
            shop_at (fd, player, -order->param1);
          break;
        case 'd':               /* diplomatic, done elsewhere */
          break;
        case 'D':               /* Disable */
          disable (fd, player, order->param1);
          break;
        case 'e':               /* exploration */
          if (order->param1 > 0)
            explore (fd, player, order->param1);
          else if (order->param1 < 0)
            choose (fd, player, -order->param1, order->param2 - 'A');
          break;
        case 'E':               /* evil */
          make_evilpedos (fd, player, order->param1, order->param2);
          break;
        case 'f':               /* flag */
          break;
        case 'G':               /* bounty hunting */
          bounty (fd, player, order->param1);
          break;
        case 'H':               /* heal */
          if (order->param1 == 0)
            heal (fd, player);
          else
            mudd (fd, player);
          break;
        case 'I':               /* interrogate prisoner */
          interrogate (fd, player, order->param1);
          break;
        case 'j':               /* move to other star system */
          break;
        case 'k':               /* key authentication */
          break;
        case 'K':               /* cure */
          cure (fd, player);
          break;
        case 'l':
        case 'L':
          if (order->param1 >= 0)
            change_companion (fd, player, order->param1);
          else if (order->param1 == -5)
            long_range_scan (fd, player);
          else
            commune (fd, player, -order->param1 - 1);
          break;
        case 'm':
          priority (fd, player, order->param1);
          break;
        case 'M':               /* maintain */
          maintain (fd, player, items + order->param1);
          break;
        case 'n':               /* name */
          break;
        case 'o':               /* wild rumours */
          break;
        case 'P':               /* Presidential command */
          //                              political_command(player, order->param1, order->param2);
          break;
        case 'p':               /* press */
          break;
        case 'q':               /* rumour */
          break;
        case 'R':               /* repair */
          repair (fd, player, items + order->param1);
          break;
        case 'N':
        case 'O':
        case 's': /* popcorn sales handled separately */
          break;
        case 'T':               /* training */
          train (fd, player, order->param1);
          break;
        case 'U':              /* destabilize/Unstabilize */
          destabilize (fd, player, order->param1);
          break;
        case 'V':               /* try password */
          loc = star_has_loc (player->star, terminal);
          if ((loc == NO_LOCATION)
              || ((player->experience[science] & (1 << loc)) == 0))
            {
              fprintf (fd, "<li class=\"officer\">No access to starnet here</li>\n");
              break;
            }
          fprintf (fd, "<li class=\"officer\">Trying password %s:\n", password (order->param1));
          if (order->param1 == password_key)
            {
              if (player->skills[order->param1 / 4] & 0x80)
                {
                  player->passwd_flags |= 1 << order->param1;
                  consolidate_artifacts ();
                }
              else
                player->skills[order->param1 / 4] |= 0x80;
              fprintf (fd, "<br>Password is Correct!</li>\n");
            }
          else
            fprintf (fd, "<br>Password is Wrong!</li>\n");
          break;
        case 'W':               /* collect ring */
          fprintf (fd, "<li class=\"officer\">Collected %s!</li>\n",
                   ring_string (locations[order->param1].ring));
          printf ("%s collecting %s ring!\n",
                  player->name, ring_string (locations[order->param1].ring));
          player->rings_held |= locations[order->param1].ring;
          locations[order->param1].ring = 0;
          break;
        case 'g':               /* excommunicate/forgive */
          if (prophets[order->param2] != player - players)
            {
              fprintf (fd,
                       "<li class=\"religion\">Prophet options no longer available</li>\n");
              break;
            }
          if (players[order->param1].heretic & (1 << order->param2))
            {
              fprintf (fd, "<li class=\"religion\">Forgave %s</li>\n",
                       name_string (players[order->param1].name));
              players[order->param1].heretic &= ~(1 << order->param2);
            }
          else
            {

              fprintf (fd, "<li class=\"religion\">Excommunicated %s</li>\n",
                       name_string (players[order->param1].name));
              players[order->param1].heretic |= (1 << order->param2);
              fprintf (times,
                       "<hr>%s is excommunicated by the Prophet of %s\n",
                       name_string (players[order->param1].name),
                       god_names[order->param2]);
            }
          break;
        case 'w':               /* web source */
          break;
        case 'x':               /* magic spell, already done */
          break;
        case 'y':               /* new player start */
          break;
        case 'z':               /* turn number */
          break;
        case 'Z':               /* game number */
          break;
        case 'a':
        case 'h':
        case 'i':
        case 'r':
        case 't':
        case 'v':
          printf ("Spare command %c used by %s!\n",
                  order->command, players[order->player].name);
          break;
        case '.':  /* rest */
          {
            struct ITEM *item;
            int officer = order->param1;
            int i;

            for (i = 2*officer ; i <= 2*officer + 1 ; i++)
              {
                item = lucky_item (player, i);
                if (item != items && item->flags & ITEM_LUCKY)
                  {
                    printf ("Found lucky item to work on for %s: %s\n",
                            name_string (player->name),
                            item_string (item));
                    if (item->reliability < 99)
                      maintain (fd, player, item);
                    else if (item->flags & ITEM_BROKEN)
                      repair (fd, player, item);
                  }
              }
          }
          break;
        case '\n':              /* tightbeam's spare lines */
          break;
        default:
          printf ("Strange order code %c by %s\n",
                  order->command, players[order->player].name);
        }
      order++;
    }
  while (rank <= last_rank);
  fprintf (fd, "</ul>\n");
  write_chosen_lists ();
  write_heretic_lists ();
  resolve_judgement ();
  check_stability ();
  /* fourth phase, movement, complicated mainly by chasing */
  for (p = 0; p < MAX_PLAYER; p++)
    {
      players[p].movement = players[p].star;
      if (strcmp (players[p].address, "nobody@localhost") == 0)
        players[p].star = NOWHERE;
    }
  do
    {
      order = orders;
      any_changes = FALSE;
      while (order->command != 'X')
        {
          if (order->command == 'j')
            {
              players[order->player].movement = order->param1;
              player = players + order->player;
              
              /* block illegal moves here to fix chasing */
              if (player->movement > MAX_STAR + MAX_PLAYER
                  && player->movement < BIG_NUMBER
                  && !any_gates (player, player->star,
                                 locations[player->movement].parameter))
                {
                  printf ("%s moving from %d to %d\n", player->name,
                          player->star, player->movement);
                  order->param1 = player->movement = player->star;
                  any_changes = TRUE;
                }
              if ((player->movement == OLYMPUS
                   && player->chosen <= OLYMPUS_SEEN
                   && player->movement != player->star)
                  && (player->star != HOLIDAY) && (player->star < MAX_STAR))
                {
                  order->param1 = player->movement = player->star;
                  printf ("Second bad, %s\n", player->name);
                  any_changes = TRUE;
                }
              if (player->movement < MAX_STAR
                  && player->movement >= 0 
                  && ! (star_seen (player, player->movement)))
                {
                  printf ("Jump to invisible star %s by %s\n",
                          star_names[player->movement],
                          player->name);
                  if (player->star >= 0 && player->star < MAX_STAR
                      &&!star_seen (player, player->star))
                    {
                      player->star = NOWHERE;
                      order->param1 = player->movement = get_random_star (player);
                    }
                  else
                    {
                      order->param1 = player->movement = player->star;
                    }
                  any_changes = TRUE;
                }
            }
          if (order->command == 'y')
            init_new_player (players + order->player, order->param1);
          order++;
        }
    }
  while (any_changes);

  for (p = 0; p < num_players; p++)
    {
      if (players[p].bid)
        if (!load_pod (items + players[p].ship, 0, 1))
          players[p].bid = 0;
      if (players[p].bid >= high_bid)
        {
          if (players[p].bid > high_bid || high_bidder == -1 ||
              effective_skill_level (players + p, weaponry) >
              effective_skill_level (players + high_bidder, weaponry))
            {
              high_bid = players[p].bid;
              high_bidder = p;
            }
        }
    }
  if (high_bidder != -1)
    printf ("Winning bid is $%d from %s\n", high_bid,
            players[high_bidder].name);
  reset_shops ();
  for (p = 0; p < num_players; p++)
    {
      open_results (&fd, players + p);
      fprintf (fd, "<!-- orders.c:810 -->");
      jump (fd, players + p, players[p].star, resolve_movement (p));
      if (players[p].medicine)
        unload_medicine (fd, players + p);
      if (p == high_bidder)
        {
          load_pod (items + players[p].ship, BASE_UNIT + current_unit, 1);
          units[current_unit].pay = high_bid;
          fprintf (fd, "<div class=\"mercs\">%s has been hired for $%d per turn</div>\n",
                   units[current_unit].name, units[current_unit].pay);
          fprintf (times,
                   "<hr><div class=\"mercs\">%s has been hired for $%d per turn</div>\n",
                   units[current_unit].name, units[current_unit].pay);
        }
      if (players[p].star == locations[current_contract].star)
        {
          int strength =
            ground_strength (players + p, locations[current_contract].sort);

          if (strength > high_strength) /* ties ? */
            {
              high_strength = strength;
              high_fighter = p;
            }
        }
    }

  if (high_fighter != -1)       /* contract taken */
    {
      players[high_fighter].energy += next_fee;
      if (next_fee > 60)
        next_fee /= 2;
      fprintf (times,
               "<hr><div class=\"mercs\">%s won the mercenary contract at %s - the fee is $%d for the next one</div>\n",
               name_string (players[high_fighter].name),
               star_names[locations[current_contract].star], next_fee);
    }
  else
    {
      if (next_fee < 960)
        next_fee *= 2;
      fprintf (times,
               "<hr><div class=\"mercs\">No-one took the mercenary contract at %s - the fee is $%d for the next one</div>\n",
               star_names[locations[current_contract].star], next_fee);
    }

  restock ();

  move_aliens ();
  move_wraiths ();
  /* all movement finished now */

  hide_systems ();
  if (turn > 1)
    end_turn ();
  if (dybuk && star < MAX_STAR)
    fprintf (times, "<HR>Manifestations of Chaos reported at %s!\n",
             star_names[dybuk->star]);
  consolidate_artifacts ();     /* allow for selling */

  for (p = 0; p < num_players; p++)
    {
      update_adventures (players + p);
      open_results (&fd, players + p);
      fprintf (fd, "<!-- orders.c:874 -->");
      if (p == high_fighter)
        fprintf (fd, "<P>You won the mercenary contract and gained $%d\n",
                 old_fee);
      show_characters (fd, players + p);
      show_experience (fd, players + p);
      fprintf (fd, "<H2>Your ");
      print_rules_link(fd, "Aliens", "Enemies");
      fprintf (fd, " are:</H2>\n");
      if (players[p].enemies)
        {
          for (enemy = 0; enemy < 32; enemy++)
            if (players[p].enemies & (1 << enemy))
              fprintf (fd, "%s \n", races[enemy].name);
        }
      else
        fprintf (fd, "None<P>\n");
      show_gif_map (fd, players + p);

      show_starsystem (fd, players + p, players[p].star);
    }
  for (p = 1; p < MAX_PLAYER; p++)
    if (players[p].reports)
      starnet_report (players + p);
  if (fd)
    fclose (fd);
}

void
parse_combat (char command, char *buffer, int player)
{
  int item;

  switch (command)
    {
    case 'c':                   /* combat option */
      players[player].strategy.cbt_option = atoi (buffer);
      break;
    case 'd':                   /* demands */
      item = atoi (buffer);
      items[item].flags |= ITEM_DEMANDED;
      players[player].strategy.demand = item;
      break;
    case 'f':                   /* photon torpedoes */
      players[player].strategy.firing_rate = atoi (buffer);
      break;
    case 'g':                   /* gift */
      item = atoi (buffer);
      if (item == -1)           /* means any */
        {
          item = players[player].ship;
          while (item)
            {
              items[item].flags |= ITEM_OFFERED;
              item = items[item].link;
            }
        }
      else
        items[item].flags |= ITEM_OFFERED;
      break;
    case 'h':                   /* hide/hunt */
      if (players[player].standby == 15)
        {
          players[player].hide_hunt = atoi (buffer);
        }
      break;
    case 'i':                   /* ideal range */
      players[player].strategy.ideal_range = atoi (buffer);
      break;
    case 'o':                   /* option */
      players[player].strategy.dip_option = atoi (buffer);
      break;
    case 'p':                   /* protects */
      item = atoi (buffer);
      items[item].flags |= ITEM_PROTECTED;
      break;
    case 'r':                   /* retreat */
      players[player].strategy.retreat = atoi (buffer);
      break;
    case 't':                   /* targets */
      item = atoi (buffer);
      items[item].flags |= ITEM_TARGETTED;
      break;
    default:
      printf ("Strange combat order (%c)\n", command);
      break;
    }
}

void
set_default_orders ()
{
  int player;
  struct PLAYER *p;

  for (player = 0; player < MAX_PLAYER; player++)
    {
      p = players + player;
      p->strategy.dip_option = flee;
      p->strategy.cbt_option = favour_fleeing;
      p->strategy.ideal_range =
        find_longest_weapon (items + players[player].ship);
      if (p->strategy.ideal_range == 255)       /* no weapons */
        p->strategy.ideal_range = 6;
      p->strategy.demand = 0;
      p->strategy.retreat = 1;
      p->movement = p->star;
    }
}

void
invent_order (struct PLAYER *player, skill_sort sort)
{
  char buffer[80];
  struct ITEM *item;
  int maintained_item = 0;
  int maintainable_item = 0;
  int maintenance_points = 0;
  int priority_points = 0;
  int priority_items = 0;
  int fixable_item = 0;
  int fixable_tech = 0;
  sprintf (buffer, "%c=.", skill_names[sort][0]);

  if (player == dybuk)
    {
      generate_evil_voices (0, player, sort);
    }
  else if (player->crew[sort]
           && (player->pools[sort] <
               skill_level (player->skills[sort])
               * skill_level (player->skills[sort]))
           && (player->pools[sort] / player->crew[sort] <
               skill_level (player->skills[sort])))
    {
      sprintf (buffer, "%c=T%d", skill_names[sort][0], sort);
    }
  else
    {
      for (item = items + player->ship ;
           item != items && item->sort < pod ;
           item = items + item->link)
        {
          int max_gain = min (effective_skill_level (player, sort) -
                              item->efficiency * item->efficiency,
                              99 - item->reliability);
          int weighted_gain = 0;
          if (repairers[item->sort] == sort
              && item->reliability == 99
              && item->efficiency * item->efficiency  <
                 effective_skill_level (player, sort)
              && item->flags & ITEM_BROKEN
              && (!maintained_item
                  || (item->efficiency > items[maintained_item].efficiency)
                  || (   (item->efficiency == items[maintained_item].efficiency)
                      && (item->collection > items[maintained_item].collection))))
            {
              maintained_item = item - items;
            }
          weighted_gain =  4 * item->efficiency * max_gain + item->collection;
          if (repairers[item->sort] == sort 
              && !(item->flags & ITEM_DEMO)
              && weighted_gain > maintenance_points)
            {
              maintenance_points = weighted_gain;
              maintainable_item = item - items;
              if (priority_items < effective_skill_level(player, sort)/4)
                {
                  priority_items++;
                  priority_points += item->efficiency *
                    min (max_gain,
                         max ( ( effective_skill_level (player,sort) -
                                 item->reliability),
                               0));
                }
            }
          if (repairers[item->sort] == sort && item->flags & ITEM_BROKEN
              && item->efficiency > fixable_tech)
            {
              fixable_tech = item->efficiency;
              fixable_item = item - items;
            }
        }
      if (maintained_item)
        sprintf (buffer, "%c=R%d", skill_names[sort][0], maintained_item);
      else if (priority_points > maintenance_points/4)
        sprintf (buffer, "%c=m", skill_names[sort][0]);
      else if (maintainable_item)
        sprintf (buffer, "%c=M%d", skill_names[sort][0], maintainable_item);
      else if (fixable_item)
        sprintf (buffer, "%c=R%d", skill_names[sort][0], fixable_item);
      else
        switch (sort)
          {
          case medical:
            if (player->health < 999)
              sprintf (buffer, "%c=H0", skill_names[sort][0]);
            break;
          case science:
            sprintf (buffer, "%c=L-5", skill_names[sort][0]);
            break;
          case weaponry:
            if (player->prisoner)
              sprintf (buffer, "%c=I0", skill_names[sort][0]);
            break;
          case engineering:
            break;
          }
    }
  parse_order (buffer, player - players);
}

/* returns TRUE if it read real orders, FALSE if it made them up */
int
read_orders (int player, int turn)
{
  FILE *fd;
  char buffer[1024];
  int i, loc;
  skill_sort sort;
  char *name = players[player].name;

  sprintf (buffer, "%s/orders/%d/%s%d", webroot, game, name, turn);
  if (want_verbose)
    fprintf (stdout, "checking %s for orders... ", buffer);
  fd = fopen (buffer, "r");
  if (!fd)
    {
      if (want_verbose)
        fprintf (stdout, "nope!\n");
      if (turn)
        create_header (players + player);
      if (turn <= 1)
        return (FALSE);
      if (players[player].last_orders == 0)
        {
          fprintf (report, "%s (%d) never made a turn, missed %d\n",
                   players[player].name,
                   player, turn - players[player].last_restart);
          if (turn - players[player].last_restart > 3)
            parse_order ("y=-1", player);
        }
      else if (turn - players[player].last_orders > 3)
        fprintf (report, "%s (%d) started on %d, missed %d (%s)\n",
                 players[player].name,
                 player,
                 players[player].last_restart,
                 turn - players[player].last_orders,
                 players[player].star == HOLIDAY
                 || players[player].star >=
                 MAX_STAR ? "on holiday" : "no excuse");
      if (players[player].account_number == 0)
        fprintf (report, "%s (%d) already dropped out\n",
                 name, players[player].account_number);
      if (players[player].preferences & 32)     /* restart */
        {
          parse_order ("y=1", player);
          return (TRUE);
        }
      if (turn - players[player].last_orders > 6
          && players[player].star < MAX_STAR
          && players + player != dybuk)
        {
          printf ("%s forced on holiday\n", players[player].name);
          sprintf (buffer, "j=%d", player + MAX_STAR);
        }
      else
        sprintf (buffer, "j=%d", players[player].star);
      parse_order (buffer, player);
      for (sort = engineering; sort <= weaponry; sort++)
        invent_order (players + player, sort);
      for (i = 0; i < MAX_LOCATION; i++)
        {
          loc = dice (MAX_LOCATION);
          if (locations[loc].star == players[player].star &&
              loc_type (loc, LOC_EXPLORABLE) &&
              loc_accessible (players + player, loc))
            {
              sprintf (buffer, "e=%d", loc);
              parse_order (buffer, player);
              return (FALSE);
            }
        }
      return (FALSE);
    }
  if (turn)
    {
      if (want_verbose)
        fprintf (stdout, "yep!\n");
      players[player].got_some_orders = TRUE;
    }


  players[player].preferences = 0;      /* override old with new */
  if (players[player].star > MAX_STAR)
    {
      printf ("%s put in orders while on holiday, returning to star %d\n",
              players[player].name, players[player].old_star);
      sprintf (buffer, "j=%d", players[player].old_star);
      parse_order (buffer, player);
    }
  while (fgets (buffer, 1000, fd))
    {
      if (buffer[1] != '=' && buffer[2] != '=')
        continue;
      if (buffer[strlen (buffer) - 2] == '=')
        continue;
      if (turn == 0 && *buffer != 'z')
        continue;
      parse_order (buffer, player);
      while (strchr (buffer, ' ') &&
             *buffer != 'p' && *buffer != 'f' &&
             *buffer != 'q' && *buffer != 'o')
        {
          strcpy ((char *) (strchr (buffer, '=') + 1),
                  (char *) (strchr (buffer, ' ') + 1));
          parse_order (buffer, player);
        }
    }
  fclose (fd);
  if (players[player].preferences & 64)
    players[player].password = rand32();
  if (turn)
    create_header (players + player);
  return (TRUE);
}

