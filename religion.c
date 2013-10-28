#include "globals.h"
#include "religion.h"
#include "util.h"
#include "items.h"
#include "locations.h"
#include "dybuk.h"
#include "rand.h"
#include "tbg-big.h"
#include "skill.h"
#include "tbg.h"
#include "terminals.h"
#include "adventures.h"

void
add_favour (struct PLAYER *player, skill_sort skill, int amount)
{
  if (player->heretic & (1 << skill))
    favours[skill] -= amount;
  else if (player->chosen & (1 << skill))
    favours[skill] += 2 * amount;
  else
    favours[skill] += amount;
}

struct SPELL
{
  int cost;
  skill_sort skill;
  int popcorn;
  int evil;
}
  spells[] =
    {
      00, -1, 0, 0,                       /* no spell */
      30, science, 0, 0,                  /* MAGIC_REPORT_SOME */
      25, science, 0, 0,                  /* MAGIC_PURGE_RIVALS */
      60, science,  0, 0,                 /* MAGIC_REPORT_ALL */
      60, engineering,  0, 0,             /* MAGIC_FAKE_KEY */
      75, medical,  0, 0,                 /* MAGIC_PACIFY */
      45, engineering, 0, 0,              /* MAGIC_BLESS_WARP */
      45, engineering, 0, 0,              /* MAGIC_BLESS_IMPULSE */
      45, science, 0, 0,                  /* MAGIC_BLESS_SENSOR */
      45, science, 0, 0,                  /* MAGIC_BLESS_CLOAK */
      45, medical, 0, 0,                  /* MAGIC_BLESS_LIFE */
      45, medical, 0, 0,                  /* MAGIC_BLESS_SICKBAY */
      45, weaponry, 0, 0,                 /* MAGIC_BLESS_SHIELD */
      45, weaponry, 0, 0,                 /* MAGIC_BLESS_WEAPON */
      70, engineering, 0, 0,              /* MAGIC_UNCURSE_WARP */
      70, engineering, 0, 0,              /* MAGIC_UNCURSE_IMPULSE */
      70, science, 0, 0,                  /* MAGIC_UNCURSE_SENSOR */
      70, science, 0, 0,                  /* MAGIC_UNCURSE_CLOAK */
      70, medical, 0, 0,                  /* MAGIC_UNCURSE_LIFE */
      70, medical, 0, 0,                  /* MAGIC_UNCURSE_SICKBAY */
      70, weaponry, 0, 0,                 /* MAGIC_UNCURSE_SHIELD */
      70, weaponry, 0, 0,                 /* MAGIC_UNCURSE_WEAPON */
      20, engineering, 0, 0,              /* MAGIC_CHARM_ENGINEERING */
      20, science, 0, 0,                  /* MAGIC_CHARM_SCIENCE */
      20, medical, 0, 0,                  /* MAGIC_CHARM_MEDICAL */
      20, weaponry, 0, 0,                 /* MAGIC_CHARM_WEAPONARY */
      50, weaponry, 0, 0,                 /* MAGIC_GROUND_COMBAT */
      100, engineering, 0, 0,             /* MAGIC_ENGINEERING_ENLIGHTENMENT */
      100, science, 0, 0,                 /* MAGIC_SCIENCE_ENLIGHTENMENT */
      100, medical, 0, 0,                 /* MAGIC_MEDICAL_ENLIGHTENMENT */
      100, weaponry, 0, 0,                /* MAGIC_WEAPONRY_ENLIGHTENMENT */
      100, engineering, 0, 0,             /* MAGIC_ENGINEERING_PROPHET */
      100, science, 0, 0,                 /* MAGIC_SCIENCE_PROPHET */
      100, medical, 0, 0,                 /* MAGIC_MEDICAL_PROPHET */
      100, weaponry, 0, 0,                /* MAGIC_WEAPONRY_PROPHET */
      0, engineering, 0, 0,               /* MAGIC_ENGINEERING_RETIRE */
      0, science, 0, 0,                   /* MAGIC_SCIENCE_RETIRE */
      0, medical, 0, 0,                   /* MAGIC_MEDICAL_RETIRE */
      0, weaponry, 0, 0,                  /* MAGIC_WEAPONRY_RETIRE */
      15, engineering, 0, -5,              /* MAGIC_ENGINEERING_PRAISE */
      15, science, 0, -5,                  /* MAGIC_SCIENCE_PRAISE */
      15, medical, 0, -5,                  /* MAGIC_MEDICAL_PRAISE */
      15, weaponry, 0, -5,                 /* MAGIC_WEAPONRY_PRAISE */
      -15, engineering, 0, 5,             /* MAGIC_ENGINEERING_DENOUNCE */
      -15, science, 0, 5,                 /* MAGIC_SCIENCE_DENOUNCE */
      -15, medical, 0, 5,                 /* MAGIC_MEDICAL_DENOUNCE */
      -15, weaponry, 0, 5,                /* MAGIC_WEAPONRY_DENOUNCE */
      200, engineering, 0, 0,             /* MAGIC_PROTECT_SHIP */
      200, science, 0, 0,                 /* MAGIC_CONCEAL_EVIL */
      200, medical, 0, 0,                 /* MAGIC_PROTECT_CREW */
      200, weaponry, 0, -100,             /* MAGIC_BANISH_EVIL */
      0, science, 0, 100,                 /* MAGIC_RELEASE_EVIL */
      75, science, 0, 0,                  /* MAGIC_TRACE_SHIP */
      25, science, 0, 0,                  /* MAGIC_VIEW_TRACE */
      50, science, 0, 0,                  /* MAGIC_REMOVE_TRACE */
      30, engineering, 0, 0,              /* MAGIC_ENGINEERING_IMPROVE */
      30, science, 0, 0,                  /* MAGIC_SCIENCE_IMPROVE */
      30, medical, 0, 0,                  /* MAGIC_MEDICAL_IMPROVE */
      30, weaponry, 0, 0,                 /* MAGIC_WEAPONRY_IMPROVE */
      100, engineering, 0, 0,             /* MAGIC_ENGINEERING_COLLECT_RING */
      100, science, 0, 0,                 /* MAGIC_SCIENCE_COLLECT_RING */
      100, medical, 0, 0,                 /* MAGIC_MEDICAL_COLLECT_RING */
      100, weaponry, 0, 0,                /* MAGIC_WEAPONRY_COLLECT_RING */
      40, science, 0, 0,                  /* MAGIC_HIDE_SYSTEM */
      25, engineering, 0, 0,              /* MAGIC_AVOID_COMBAT */
      25, weaponry, 0, 0,                 /* MAGIC_FORCE_COMBAT */
      25, medical, 0, 0,                  /* MAGIC_PROTECT_FROM_COMBAT */
      10, science, 0, 0,                  /* MAGIC_REPORT_ADVENTURE */
      35, engineering, 0, 0,              /* MAGIC_POWER_UP */
      50, engineering, 0, 0,              /* MAGIC_POWER_DOWN */
      20, medical, 0, 0,                  /* MAGIC_BLESS_MEDICAL */
      15, science, 0, 0,                  /* MAGIC_SUMMON_POPCORN */
      20, medical, 0, 0,                  /* MAGIC_STIMULATE */
      40, science, 0, 0,                  /* MAGIC_SET_PROBE */
      20, science, 0, 0,                  /* MAGIC_USE_PROBE */
      40, science, 0, 0,                  /* MAGIC_CLEAR_PROBES */
      10, engineering, 0, 0,              /* MAGIC_LUCKY_WARP */   
      10, engineering, 0, 0,              /* MAGIC_LUCKY_IMPULSE */
      10, science, 0, 0,                  /* MAGIC_LUCKY_SENSOR */ 
      10, science, 0, 0,                  /* MAGIC_LUCKY_CLOAK */  
      10, medical, 0, 0,                  /* MAGIC_LUCKY_SICKBAY */   
      10, medical, 0, 0,                  /* MAGIC_LUCKY_LIFESUPPORT */
      10, weaponry, 0, 0,                 /* MAGIC_LUCKY_SHIELD */ 
      10, weaponry, 0, 0,                 /* MAGIC_LUCKY_WEAPON */
      25, engineering, 5, 10,             /* MAGIC_SUPER_ENGINEERING */
      25, science, 5, 10,                 /* MAGIC_SUPER_SCIENCE */
      25, medical, 5, 10,                 /* MAGIC_SUPER_MEDICAL */
      25, weaponry, 5, 10,                /* MAGIC_SUPER_WEAPONRY */
      100, engineering, 0, 0,             /* MAGIC_ATONE_ENGINEERING */
      100, science, 0, 0,                 /* MAGIC_ATONE_SCIENCE */
      100, medical, 0, 0,                 /* MAGIC_ATONE_MEDICAL */
      100, weaponry, 0, 0,                /* MAGIC_ATONE_WEAPONRY */
      50, science, 5, 10,                 /* MAGIC_REVEAL_EVIL */
    };

int
spell_valid (struct PLAYER *player, int spell)
{
  return (((player->favour[spells[spell].skill] >= spells[spell].cost)
           || (player->chosen & (1 << spells[spell].skill))
           || (spells[spell].cost == 0))
          && (player->popcorn >= spells[spell].popcorn
              || player == dybuk));
}

void
generate_magic_options (FILE * fd, struct PLAYER *player,
                        skill_sort sort, struct PLAYER *enemy)
{
  int i, loc, key, p;
  item_sort type;
  struct ITEM *item;
  int cost;

  for (type = warp_drive + sort * 2; type <= warp_drive + sort * 2 + 1;
       type++)
    {
      item = items + player->ship;
      while (item != items)
        {
          if (item->sort == artifact)
            if ((item->magic & (0x100 << type))
                && spell_valid (player, MAGIC_UNCURSE_WARP + type))
              {
                fprintf (fd, "<OPTION VALUE=%d>Remove all %s curses (%d)\n",
                         MAGIC_UNCURSE_WARP + type, short_item_names[type],
                         spells[MAGIC_UNCURSE_WARP + type].cost);
                break;
              }
          item = items + item->link;
        }
    }

  for (type = warp_drive; type <= ram; type++)
    if (repairers[type] == sort)
      {
        item = lucky_item (player, type);

        if (spell_valid (player, MAGIC_LUCKY_WARP + type) && item != items)
          fprintf (fd, "<OPTION VALUE=%d>Lucky %s (%d)\n",
                   MAGIC_LUCKY_WARP + type,
                   item_string (item), spells[MAGIC_LUCKY_WARP + type].cost);
      }
  loc = star_has_ring (player->star, 4 + sort);
  if (loc && (player->rings_seen & locations[loc].ring) &&
      spell_valid (player, MAGIC_ENGINEERING_COLLECT_RING + sort))
    fprintf (fd, "<OPTION VALUE=%d>Collect Good Ring (%d)\n",
             MAGIC_ENGINEERING_COLLECT_RING + sort,
             spells[MAGIC_ENGINEERING_COLLECT_RING + sort].cost);

  if (star_has_loc (player->star, hall) != NO_LOCATION &&
      spell_valid (player, MAGIC_CHARM_ENGINEERING + sort))
    fprintf (fd, "<OPTION VALUE=%d>Charm %s Recruits (%d)\n",
             MAGIC_CHARM_ENGINEERING + sort, skill_names[sort],
             spells[MAGIC_CHARM_ENGINEERING].cost);

  if (spell_valid (player, MAGIC_ENGINEERING_ENLIGHTENMENT + sort) &&
      !enlightened (player, sort))
    fprintf (fd, "<OPTION VALUE=%d>Enlightenment (%d)\n",
             MAGIC_ENGINEERING_ENLIGHTENMENT + sort,
             spells[MAGIC_ENGINEERING_ENLIGHTENMENT + sort].cost);

  if (prophets[sort] != -1 && prophets[sort] != player - players &&
      spell_valid (player, MAGIC_ENGINEERING_PRAISE + sort))
    fprintf (fd, "<OPTION VALUE=%d>Praise Prophet %s (%d)\n",
             MAGIC_ENGINEERING_PRAISE + sort,
             name_string (players[prophets[sort]].name),
             spells[MAGIC_ENGINEERING_PRAISE + sort].cost);

  cost = (player->heretic & (1 << sort)) ? 15 : -15;
  if (prophets[sort] != -1 && prophets[sort] != player - players &&
      spell_valid (player, MAGIC_ENGINEERING_DENOUNCE + sort) &&
      player->favour[sort] >= 15)
    fprintf (fd, "<OPTION VALUE=%d>Denounce Prophet %s (%d)\n",
             MAGIC_ENGINEERING_DENOUNCE + sort,
             name_string (players[prophets[sort]].name), cost);

  if (enlightened (player, sort) &&
      spell_valid (player, MAGIC_ENGINEERING_PROPHET + sort))
    fprintf (fd, "<OPTION VALUE=%d>Become Prophet (%d)\n",
             MAGIC_ENGINEERING_PROPHET + sort,
             spells[MAGIC_ENGINEERING_PROPHET + sort].cost);

  if (prophets[sort] == player - players)
    fprintf (fd, "<OPTION VALUE=%d>Retire as Prophet (%d)\n",
             MAGIC_ENGINEERING_RETIRE + sort,
             spells[MAGIC_ENGINEERING_RETIRE + sort].cost);
  if (spell_valid (player, MAGIC_SUPER_ENGINEERING + sort))
    fprintf (fd, "<OPTION VALUE=%d>Supercharge %s crew (%d)\n",
             MAGIC_SUPER_ENGINEERING + sort,
             skill_names[sort],
             spells[MAGIC_SUPER_ENGINEERING + sort].cost);
  if (spell_valid (player, MAGIC_ATONE_ENGINEERING + sort)
      && player->evil > ((player->chosen & (1 << sort)) ? 0 : 100))
    fprintf (fd, "<OPTION VALUE=%d>Atone for past misdeeds (%d)\n",
             MAGIC_ATONE_ENGINEERING + sort,
             spells[MAGIC_ATONE_ENGINEERING + sort].cost);
  switch (sort)
    {
    case engineering:
      if (spell_valid (player, MAGIC_POWER_UP))
        fprintf (fd, "<OPTION VALUE=%d>Power-up (%d)\n",
                 MAGIC_POWER_UP, spells[MAGIC_POWER_UP].cost);
      if (spell_valid (player, MAGIC_POWER_DOWN))
        fprintf (fd, "<OPTION VALUE=%d>Power-down (%d)\n",
                 MAGIC_POWER_DOWN, spells[MAGIC_POWER_DOWN].cost);
      if (spell_valid (player, MAGIC_AVOID_COMBAT) && (pairing (player)))
        fprintf (fd, "<OPTION VALUE=%d>Micro-jump Flee (%d)\n",
                 MAGIC_AVOID_COMBAT, spells[MAGIC_AVOID_COMBAT].cost);
      if (spell_valid (player, MAGIC_BLESS_WARP))
        fprintf (fd, "<OPTION VALUE=%d>Bless Warp Drive (%d)\n",
                 MAGIC_BLESS_WARP, spells[MAGIC_BLESS_WARP].cost);
      if (spell_valid (player, MAGIC_BLESS_IMPULSE))
        fprintf (fd, "<OPTION VALUE=%d>Bless Impulse Drive (%d)\n",
                 MAGIC_BLESS_IMPULSE, spells[MAGIC_BLESS_IMPULSE].cost);
      for (loc = 0; loc < MAX_LOCATION; loc++)
        {
          if (locations[loc].star == player->star &&
              locations[loc].sort == stargate)
            key = (player->star ^ locations[loc].parameter) & 7;
          else
            continue;
          if (spell_valid (player, MAGIC_FAKE_KEY))
            fprintf (fd, "<OPTION VALUE=%d%c>Improvise Key %d (%d)\n",
                     MAGIC_FAKE_KEY, key + 'A', key,
                     spells[MAGIC_FAKE_KEY].cost);
        }
      if (enlightened (player, sort) && is_evil (enemy))
        {
          if (spell_valid (player, MAGIC_PROTECT_SHIP))
            fprintf (fd, "<OPTION VALUE=%d>Protect Ship from Chaos (%d)\n",
                     MAGIC_PROTECT_SHIP, spells[MAGIC_PROTECT_SHIP].cost);
        }
      break;
    case science:
      if (player->star == popcorn.star && !dybuk)
        {
          fprintf (fd, "<OPTION VALUE=%d>Release Chaos! (?)\n",
                   MAGIC_RELEASE_EVIL);
        }
      if (player->star >= 0 && spell_valid (player, MAGIC_SET_PROBE))
        fprintf (fd, "<OPTION VALUE=%d>Deploy Probe (%d)\n",
                 MAGIC_SET_PROBE, spells[MAGIC_SET_PROBE].cost);
      if (player->probe < 0)
        player->probe = NOWHERE;

      if (spell_valid (player, MAGIC_USE_PROBE) && (player->probe != NOWHERE))
        fprintf (fd, "<OPTION VALUE=%d>Probe Report from %s (%d)\n",
                 MAGIC_USE_PROBE,
                 star_names[player->probe], spells[MAGIC_USE_PROBE].cost);
      if (spell_valid (player, MAGIC_CLEAR_PROBES))
        fprintf (fd, "<OPTION VALUE=%d>Destroy All Probes Here (%d)\n",
                 MAGIC_CLEAR_PROBES, spells[MAGIC_CLEAR_PROBES].cost);
      if (spell_valid (player, MAGIC_REPORT_ADVENTURE))
        fprintf (fd, "<OPTION VALUE=%d>Discover Adventure (%d)\n",
                 MAGIC_REPORT_ADVENTURE, spells[MAGIC_REPORT_ADVENTURE].cost);
      if (spell_valid (player, MAGIC_HIDE_SYSTEM))
        fprintf (fd, "<OPTION VALUE=%d>Hide Starsystem (%d)\n",
                 MAGIC_HIDE_SYSTEM, spells[MAGIC_HIDE_SYSTEM].cost);
      if (spell_valid (player, MAGIC_BLESS_SENSOR))
        fprintf (fd, "<OPTION VALUE=%d>Bless Sensors (%d)\n",
                 MAGIC_BLESS_SENSOR, spells[MAGIC_BLESS_SENSOR].cost);
      if (spell_valid (player, MAGIC_BLESS_CLOAK))
        fprintf (fd, "<OPTION VALUE=%d>Bless Cloaks (%d)\n",
                 MAGIC_BLESS_CLOAK, spells[MAGIC_BLESS_CLOAK].cost);
      if (star_has_loc (player->star, terminal) != NO_LOCATION
          && spell_valid (player, MAGIC_PURGE_RIVALS))
        fprintf (fd, "<OPTION VALUE=%d>Purge Rivals (%d)\n",
                 MAGIC_PURGE_RIVALS, spells[MAGIC_PURGE_RIVALS].cost);
      if (spell_valid (player, MAGIC_REPORT_ALL))
        fprintf (fd, "<OPTION VALUE=%d>Report From All Terminals (%d)\n",
                 MAGIC_REPORT_ALL, spells[MAGIC_REPORT_ALL].cost);
      if (spell_valid (player, MAGIC_REPORT_SOME))
        fprintf (fd, "<OPTION VALUE=%d>Report From Some Terminals (%d)\n",
                 MAGIC_REPORT_SOME, spells[MAGIC_REPORT_SOME].cost);
      if (enlightened (player, sort))
        {
          if (spell_valid (player, MAGIC_CONCEAL_EVIL))
            fprintf (fd, "<OPTION VALUE=%d>Conceal system from chaos (%d)\n",
                     MAGIC_CONCEAL_EVIL, spells[MAGIC_CONCEAL_EVIL].cost);
        }
      if (spell_valid (player, MAGIC_REVEAL_EVIL))
        fprintf (fd, "<OPTION VALUE=%d>Reveal system to chaos (%d)\n",
                 MAGIC_REVEAL_EVIL, spells[MAGIC_REVEAL_EVIL].cost);
      for (p = 0; p < MAX_PLAYER; p++)
        {
          if (spell_valid (player, MAGIC_TRACE_SHIP) &&
              players[p].star == player->star && p != player - players)
            fprintf (fd, "<OPTION VALUE=%d>Trace %s (%d)\n",
                     BIG_NUMBER + p,
                     name_string (players[p].name),
                     spells[MAGIC_TRACE_SHIP].cost);
        }
      if (spell_valid (player, MAGIC_VIEW_TRACE) && player->tracer != 0)
        fprintf (fd, "<OPTION VALUE=%d>View %s (%d)\n",
                 MAGIC_VIEW_TRACE,
                 name_string (players[player->tracer].name),
                 spells[MAGIC_VIEW_TRACE].cost);
      if (spell_valid (player, MAGIC_REMOVE_TRACE))
        fprintf (fd, "<OPTION VALUE=%d>Remove any Traces on %s (%d)\n",
                 MAGIC_REMOVE_TRACE,
                 name_string (player->name), spells[MAGIC_REMOVE_TRACE].cost);
      break;
    case medical:
      if (spell_valid (player, MAGIC_BLESS_MEDICAL))
        fprintf (fd, "<OPTION VALUE=%d>Bless Away Team (%d)\n",
                 MAGIC_BLESS_MEDICAL, spells[MAGIC_BLESS_MEDICAL].cost);
      if (spell_valid (player, MAGIC_BLESS_LIFE))
        fprintf (fd, "<OPTION VALUE=%d>Bless Life Support (%d)\n",
                 MAGIC_BLESS_LIFE, spells[MAGIC_BLESS_LIFE].cost);
      if (spell_valid (player, MAGIC_BLESS_SICKBAY))
        fprintf (fd, "<OPTION VALUE=%d>Bless Sickbay (%d)\n",
                 MAGIC_BLESS_SICKBAY, spells[MAGIC_BLESS_SICKBAY].cost);
      for (i = 0; i < 32; i++)
        if (player->enemies & (1 << i) && spell_valid (player, MAGIC_PACIFY))
          fprintf (fd, "<OPTION VALUE=%d%c>Pacify %s Nation (%d)\n",
                   MAGIC_PACIFY, i + 'A',
                   races[i].name, spells[MAGIC_PACIFY].cost);
      if (enlightened (player, sort) && is_evil (enemy))
        {
          if (spell_valid (player, MAGIC_PROTECT_CREW))
            fprintf (fd, "<OPTION VALUE=%d>Protect Crew from Chaos (%d)\n",
                     MAGIC_PROTECT_CREW, spells[MAGIC_PROTECT_CREW].cost);
        }
      break;
    case weaponry:
      if (spell_valid (player, MAGIC_FORCE_COMBAT) && (pairing (player)))
        fprintf (fd, "<OPTION VALUE=%d>Counter Micro-jump (%d)\n",
                 MAGIC_FORCE_COMBAT, spells[MAGIC_FORCE_COMBAT].cost);
      if (spell_valid (player, MAGIC_BLESS_SHIELD))
        fprintf (fd, "<OPTION VALUE=%d>Bless Shields (%d)\n",
                 MAGIC_BLESS_SHIELD, spells[MAGIC_BLESS_SHIELD].cost);
      if (spell_valid (player, MAGIC_BLESS_WEAPON))
        fprintf (fd, "<OPTION VALUE=%d>Bless Weapons (%d)\n",
                 MAGIC_BLESS_WEAPON, spells[MAGIC_BLESS_WEAPON].cost);
      if (spell_valid (player, MAGIC_GROUND_COMBAT))
        fprintf (fd, "<OPTION VALUE=%d>Bless Ground Combat (%d)\n",
                 MAGIC_GROUND_COMBAT, spells[MAGIC_GROUND_COMBAT].cost);
      if (enlightened (player, sort) && is_evil (enemy))
        {
          if (spell_valid (player, MAGIC_BANISH_EVIL))
            fprintf (fd, "<OPTION VALUE=%d>Banish Chaos (%d)\n",
                     MAGIC_BANISH_EVIL, spells[MAGIC_BANISH_EVIL].cost);
        }
      break;
    }
}


void
generate_prophet_options (FILE * fd, struct PLAYER *player)
{
  skill_sort sort;
  int do_some = FALSE, p;

  for (sort = engineering; sort <= weaponry; sort++)
    if (prophets[sort] == player - players)
      do_some = TRUE;
  if (!do_some)
    return;
  fprintf (fd, "<TABLE BORDER=1>\n");
  for (sort = engineering; sort <= weaponry; sort++)
    if (prophets[sort] == player - players)
      {
        fprintf (fd, "<TR><TH COLSPAN=4>As Prophet of %s</TH></TR>\n",
                 god_names[sort]);
        fprintf (fd, "<TR><TH>Choose</TH><TH>Unchoose</TH>\n");
        fprintf (fd, "<TH>Excommunicate</TH><TH>Forgive</TH></TR>\n");
        fprintf (fd, "<TR ALIGN=CENTER>\n");
        fprintf (fd, "<TD><SELECT NAME=\"e\" SIZE=4 MULTIPLE>\n");
        for (p = 0; p < MAX_PLAYER; p++)
          if (players[p].star == player->star &&
              p != player - players && (players[p].chosen & (1 << sort)) == 0)
            fprintf (fd, "<OPTION VALUE=%d%c>%s\n",
                     -p, 'A' + sort, name_string (players[p].name));
        fprintf (fd, "</SELECT></TD>\n");
        fprintf (fd, "<TD><SELECT NAME=\"e\" SIZE=4 MULTIPLE>\n");
        for (p = 0; p < MAX_PLAYER; p++)
          if (p != player - players && (players[p].chosen & (1 << sort)))
            fprintf (fd, "<OPTION VALUE=%d%c>%s\n",
                     -p, 'A' + sort, name_string (players[p].name));
        fprintf (fd, "</SELECT></TD>\n");

        fprintf (fd, "<TD><SELECT NAME=\"g\" SIZE=4>\n");
        if (!new_prophets[sort])
          for (p = 0; p < MAX_PLAYER; p++)
            if (players[p].denounced & (1 << sort))
              if ((players[p].heretic & (1 << sort)) == 0)
                fprintf (fd, "<OPTION VALUE=%d%c>%s\n",
                         p, 'A' + sort, name_string (players[p].name));
        fprintf (fd, "</SELECT></TD>\n");
        fprintf (fd, "<TD><SELECT NAME=\"g\" SIZE=4 MULTIPLE>\n");
        for (p = 0; p < MAX_PLAYER; p++)
          if (players[p].heretic & (1 << sort))
            fprintf (fd, "<OPTION VALUE=%d%c>%s\n",
                     p, 'A' + sort, name_string (players[p].name));
        fprintf (fd, "</SELECT></TD>\n");
        fprintf (fd, "</TR>\n");
      }
  fprintf (fd, "</TABLE>\n");
}

void
cast_spell (FILE * fd, struct PLAYER *player, int spell, int qualifier)
{
  int p, reward, i, total, loc, current, borrow;
  struct ITEM *item;
  skill_sort skill = spells[spell].skill;

  if (qualifier == '.')
    return;

  if (spells[spell].popcorn > 0)
    {
      if (player == dybuk)
        {
          fprintf (fd, "<BR>The forces of chaos make spell casting easier\n");
        }
      else if (player->popcorn < spells[spell].popcorn)
        {
          fprintf (fd, "<P><EM>Can't afford spell %d</EM>\n", spell);
          return;
        }
      else
        {
          player->popcorn -= spells[spell].popcorn;
        }
    }
  
  if (player->favour[skill] < spells[spell].cost)
    {
      if (prophets[skill] != -1 && player->chosen & (1 << skill)
          && prophets[skill] != player - players)
        {
          borrow = spells[spell].cost - player->favour[skill];
          players[prophets[skill]].favour[skill] -= borrow;
          player->favour[skill] += borrow;
          fprintf (fd,
                   "<BR>The Prophet %s provides %d extra favour for spell casting\n",
                   name_string (players[prophets[skill]].name), borrow);
        }
      else
        {
          fprintf (fd, "<P><EM>Can't afford spell %d</EM>\n", spell);
          return;
        }
    }
  player->favour[spells[spell].skill] -= spells[spell].cost;

  switch (spell)
    {
    case MAGIC_NO_SPELL:
      break;
    case MAGIC_REPORT_SOME:
      player->reports = player->experience[science];
      while (bitcount(player->reports) > 5)
        {
          p = dice(32);
          player->reports &= ~(1 << p);
        }
      if (player->reports)
        {
          fprintf (fd,
                   "<P>Requested reports from some terminals, see below\n");
        }
      else
        {
          fprintf (fd,
                   "<P>Requested reports from some terminals, unable to access any\n");
        }
      break;
    case MAGIC_PURGE_RIVALS:
      purge_accounts (player);
      fprintf (fd,
               "<P>Other players' Starnet accounts purged on %s terminal\n",
               star_names[player->star]);
      break;
    case MAGIC_REPORT_ALL:
      for (p = 0 ; p < 32; p++)
        if (dice(100) < effective_skill_level(player,science))
          player->reports |= (1 << p);
      player->reports &= player->experience[science];
      if (player->reports)
        {
          fprintf (fd,
                   "<P>Requested reports from all accessible terminals, see below\n");
        }
      else
        {
          fprintf (fd,
                   "<P>Requested reports from all accessible terminals, unable to access any\n");
        }
      break;
    case MAGIC_FAKE_KEY:
      player->blessings |= 1 << qualifier;
      fprintf (fd, "<P>Key %d improvised\n", qualifier);
      break;
    case MAGIC_PACIFY:
      player->enemies &= ~(1 << qualifier);
      fprintf (fd,
               "<P>Medical officer saved the life of %s ambassador and they make peace\n",
               races[qualifier].name);
      break;
    case MAGIC_BLESS_WARP:
    case MAGIC_BLESS_IMPULSE:
    case MAGIC_BLESS_SENSOR:
    case MAGIC_BLESS_CLOAK:
    case MAGIC_BLESS_LIFE:
    case MAGIC_BLESS_SICKBAY:
    case MAGIC_BLESS_SHIELD:
    case MAGIC_BLESS_WEAPON:
      player->blessings |= 0x10000 << (spell - MAGIC_BLESS_WARP);
      fprintf (fd,
               "<P>All %ss working at increased efficiency throughout turn\n",
               spell ==
               MAGIC_BLESS_WEAPON ? "weapon" : item_names[spell -
                                                          MAGIC_BLESS_WARP]);
      break;
    case MAGIC_UNCURSE_WARP:
    case MAGIC_UNCURSE_IMPULSE:
    case MAGIC_UNCURSE_SENSOR:
    case MAGIC_UNCURSE_CLOAK:
    case MAGIC_UNCURSE_LIFE:
    case MAGIC_UNCURSE_SICKBAY:
    case MAGIC_UNCURSE_SHIELD:
    case MAGIC_UNCURSE_WEAPON:
      item = items + player->ship;
      while (item != items)
        {
          item->magic &= ~(0x100 << (spell - MAGIC_UNCURSE_WARP));
          item = items + item->link;
        }
      fprintf (fd, "<P>All %s curses removed from artifacts\n",
               spell == MAGIC_UNCURSE_WEAPON ? "weapon" :
               item_names[spell - MAGIC_UNCURSE_WARP]);
      break;
    case MAGIC_CHARM_ENGINEERING:
    case MAGIC_CHARM_SCIENCE:
    case MAGIC_CHARM_MEDICAL:
    case MAGIC_CHARM_WEAPONARY:
      player->blessings |= 0x1000000 << (spell - MAGIC_CHARM_ENGINEERING);
      fprintf (fd,
               "<P>%s officer impresses new recruits and gets access to better candidates\n",
               skill_names[spell - MAGIC_CHARM_ENGINEERING]);
      break;
    case MAGIC_GROUND_COMBAT:
      player->blessings |= GROUND_COMBAT_BIT;
      fprintf (fd,
               "<P>Weaponry officer exerts special skills in ground combat\n");
      break;
    case MAGIC_ENGINEERING_ENLIGHTENMENT:
    case MAGIC_SCIENCE_ENLIGHTENMENT:
    case MAGIC_MEDICAL_ENLIGHTENMENT:
    case MAGIC_WEAPONRY_ENLIGHTENMENT:
      player->skills[spell - MAGIC_ENGINEERING_ENLIGHTENMENT] |= 0x40;
      fprintf (fd, "<P>%s officer achieves enlightenment\n",
               skill_names[spell - MAGIC_ENGINEERING_ENLIGHTENMENT]);
      fprintf (fd, "<BR>%s appears to you and says \"",
               god_names[spell - MAGIC_ENGINEERING_ENLIGHTENMENT]);
      switch (spell - MAGIC_ENGINEERING_ENLIGHTENMENT)
        {
        case engineering:
          fprintf (fd, "I will provide a way to protect your ship\"\n");
          break;
        case science:
          fprintf (fd,
                   "I will provide a way to predict the actions of Chaos\"\n");
          break;
        case medical:
          fprintf (fd, "I will provide a way to protect your crew\"\n");
          break;
        case weaponry:
          fprintf (fd, "I will provide a way to banish Chaos\"\n");
          break;
        }
      break;
    case MAGIC_ENGINEERING_PROPHET:
    case MAGIC_SCIENCE_PROPHET:
    case MAGIC_MEDICAL_PROPHET:
    case MAGIC_WEAPONRY_PROPHET:
      skill = spell - MAGIC_ENGINEERING_PROPHET;
      current = prophets[skill];
      if (current == player - players)
        {
          printf ("Discarding prophet command\n");
          break;
        }
      if (current != -1
          && players[current].favour[skill] > player->favour[skill])
        {
          if (new_prophets[skill])
            {
              fprintf (fd,
                       "<P><EM>There's already a Prophet of %s with more favour than you</EM>\n",
                       god_names[skill]);
              player->favour[skill] += spells[spell].cost;
            }
          else
            {
              fprintf (fd,
                       "<P>Challenged %s to be Prophet of %s, each lose %d favour\n",
                       name_string (players[current].name), god_names[skill],
                       spells[spell].cost);
              fprintf (times,
                       "<HR><FONT COLOR=\"RED\">Schism! %s challenges the prophet of %s</FONT>\n",
                       name_string (player->name), god_names[skill]);
              players[current].favour[skill] -= spells[spell].cost;
              player->denounced |= 1 << (spell - MAGIC_ENGINEERING_PROPHET);
            }
          printf ("%s failed to become prophet\n", player->name);
          break;
        }
      printf ("%s becomes prophet\n", player->name);
      if (new_prophets[skill])
        players[current].favour[skill] += 100;
      if (current != -1)
        retire_prophet (skill);
      prophets[skill] = player - players;
      player->chosen |= 1 << (skill);
      fprintf (fd, "<P>You become Prophet of %s!\n", god_names[skill]);
      fprintf (times, "<HR>%s becomes Prophet of %s\n",
               name_string (player->name), god_names[skill]);
      if (current != -1)
        fprintf (times, " (replacing %s)\n",
                 name_string (players[current].name));
      new_prophets[skill] = TRUE;
      break;
    case MAGIC_ENGINEERING_RETIRE:
    case MAGIC_SCIENCE_RETIRE:
    case MAGIC_MEDICAL_RETIRE:
    case MAGIC_WEAPONRY_RETIRE:
      skill = spell - MAGIC_ENGINEERING_RETIRE;
      if (prophets[skill] != player - players)
        break;
      retire_prophet (skill);
      i = fuzz(3 * player->favour[skill])/4;
      fprintf (fd, "<P>You retire as Prophet of %s, at a cost of %d favour!\n",
               god_names[spell - MAGIC_ENGINEERING_RETIRE], i);
      player->favour[skill] -= i;
      fprintf (times, "<HR>%s retires as Prophet of %s!\n",
               name_string (player->name),
               god_names[spell - MAGIC_ENGINEERING_RETIRE]);
      break;
    case MAGIC_ENGINEERING_PRAISE:
    case MAGIC_SCIENCE_PRAISE:
    case MAGIC_MEDICAL_PRAISE:
    case MAGIC_WEAPONRY_PRAISE:
      skill = spell - MAGIC_ENGINEERING_PRAISE;
      if (prophets[spell - MAGIC_ENGINEERING_PRAISE] == -1)
        {
          fprintf (fd, "<P><EM>There is no longer a Prophet of %s</EM>\n",
                   god_names[spell - MAGIC_ENGINEERING_PRAISE]);
          player->favour[skill] += spells[spell].cost;
          break;
        }
      if (new_prophets[skill])
        {
          player->favour[skill] += spells[spell].cost;
          break;
        }
      players[prophets[skill]].favour[skill] += 15;
      fprintf (fd, "<P>You praised %s, Prophet of %s\n",
               name_string (players
                            [prophets[spell - MAGIC_ENGINEERING_PRAISE]].
                            name),
               god_names[spell - MAGIC_ENGINEERING_PRAISE]);
      fprintf (times, "<HR><FONT COLOR=\"GREEN\">%s praised\n",
               name_string (player->name));
      fprintf (times, " %s, Prophet of %s</FONT>\n",
               name_string (players
                            [prophets[spell - MAGIC_ENGINEERING_PRAISE]].
                            name),
               god_names[spell - MAGIC_ENGINEERING_PRAISE]);
      break;
    case MAGIC_ENGINEERING_IMPROVE:
    case MAGIC_SCIENCE_IMPROVE:
    case MAGIC_MEDICAL_IMPROVE:
    case MAGIC_WEAPONRY_IMPROVE:
      fprintf (fd, "<P>Improved all %s modules\n",
               skill_names[spell - MAGIC_ENGINEERING_IMPROVE]);
      item = items + player->ship;
      while (item != items && item->sort < pod)
        {
          /* EEM: changed to decrease reliability, just if anyone
             cheats */
          item->reliability = 0;
          item = items + item->link;
        }
      break;
    case MAGIC_ENGINEERING_DENOUNCE:
    case MAGIC_SCIENCE_DENOUNCE:
    case MAGIC_MEDICAL_DENOUNCE:
    case MAGIC_WEAPONRY_DENOUNCE:
      skill = spell - MAGIC_ENGINEERING_DENOUNCE;
      if (prophets[skill] == -1 || new_prophets[skill])
        {
          fprintf (fd, "<P><EM>There is no longer a Prophet of %s</EM>\n",
                   god_names[skill]);
          player->favour[spells[spell].skill] += spells[spell].cost;
          break;
        }
      players[prophets[skill]].favour[skill] -= 15;
      fprintf (fd, "<P>You denounced %s, Prophet of %s\n",
               name_string (players[prophets[skill]].name), god_names[skill]);
      fprintf (times, "<HR><FONT COLOR=\"RED\">%s denounced\n",
               name_string (player->name));
      fprintf (times, " %s, Prophet of %s</FONT>\n",
               name_string (players[prophets[skill]].name), god_names[skill]);
      player->denounced |= 1 << skill;
      if (player->heretic & (1 << skill))
        player->favour[skill] += 2 * spells[spell].cost;
      break;
    case MAGIC_PROTECT_SHIP:
      fprintf (fd, "<P>%s helps protect your ship!\n",
               god_names[engineering]);
      player->blessings |= PROTECT_SHIP_BIT;
      break;
    case MAGIC_CONCEAL_EVIL:
      fprintf (fd,
               "<P>%s casts a cloak of concealment around the system!\n",
               god_names[science]);
      break;
    case MAGIC_REVEAL_EVIL:
      fprintf (fd,
               "<P>%s lifts their cloak of concealment from the system!\n",
               god_names[science]);
      break;
    case MAGIC_PROTECT_CREW:
      fprintf (fd, "<P>%s helps protect your crew!\n", god_names[medical]);
      player->blessings |= PROTECT_CREW_BIT;
      break;
    case MAGIC_BANISH_EVIL:
      fprintf (fd, "<P>%s helps you banish Chaos!\n", god_names[weaponry]);
      player->blessings |= BANISH_EVIL_BIT;
      break;
    case MAGIC_RELEASE_EVIL:
      {
        if (dybuk)
          {
            fprintf (fd, "<EM>Chaos is already free!</EM>\n");
            break;
          }
        printf ("Chaos released!\n");
        fprintf (times, "<HR>Chaos is once again unchained!\n");
        reward = popcorn.reward;
        fprintf (fd,
                 "<P>You have released Chaos, and are rewarded with %d popcorn!\n",
                 reward);
        player->popcorn += reward;
        player->evil += 500;
        assign_dybuk ();
        reset_popcorn ();
        break;

      }

    case MAGIC_TRACE_SHIP:
      fprintf (fd, "<P>Tracer attached to %s\n",
               name_string (players[qualifier].name));
      player->tracer = qualifier;
      break;
    case MAGIC_VIEW_TRACE:
      fprintf (fd, "<P>%s viewed via tracer (report below)\n",
               name_string (players[player->tracer].name));
      player->viewing_trace = TRUE;
      break;
    case MAGIC_REMOVE_TRACE:
      total = 0;
      for (p = 0; p < MAX_PLAYER; p++)
        {
          if (players[p].tracer == player - players)
            {
              players[p].tracer = 0;
              total++;
            }
        }
      fprintf (fd, "<P>All (%d) tracers removed\n", total);
      break;
    case MAGIC_ENGINEERING_COLLECT_RING:
    case MAGIC_SCIENCE_COLLECT_RING:
    case MAGIC_MEDICAL_COLLECT_RING:
    case MAGIC_WEAPONRY_COLLECT_RING:
      printf ("%s collecting ring\n", player->name);
      loc = star_has_ring (player->star,
                           4 + spell - MAGIC_ENGINEERING_COLLECT_RING);
      if (!loc)
        fprintf (fd, "<P><EM>No Good Ring here</EM>\n");
      else
        {
          fprintf (fd, "<P>Collected Good Ring!\n");
          player->rings_held |= locations[loc].ring;
          locations[loc].ring = 0;
        }
      break;
    case MAGIC_HIDE_SYSTEM:
      player->magic_flags |= FLAG_HIDE_SYSTEM;
      fprintf (fd, "<P>Starsystem hidden from remote sensing\n");
      break;
    case MAGIC_AVOID_COMBAT:
      fprintf (fd, "<P>Tried to micro-jump to avoid combat,\n");
      if (factor (warp_drive, player) > dice (100))
        {
          player->magic_flags |= FLAG_AVOID_COMBAT;
          fprintf (fd, "<BR>and jump succeeds\n");
        }
      else
        fprintf (fd, "<EM><BR>and jump fails</EM>\n");
      break;
    case MAGIC_FORCE_COMBAT:
      player->magic_flags |= FLAG_FORCE_COMBAT;
      fprintf (fd, "<P>Ready to counter enemy micro-jump\n");
      break;
    case MAGIC_PROTECT_FROM_COMBAT:
      player->magic_flags |= FLAG_PROTECT_CREW;
      fprintf (fd, "<P>Crew protected from combat wounds\n");
      break;
    case MAGIC_REPORT_ADVENTURE:
      find_adventure (fd, player);
      break;
    case MAGIC_POWER_UP:
      player->newpowermod = 1;
      fprintf (fd, "<P>Made the ship look more powerful\n");
      break;
    case MAGIC_POWER_DOWN:
      player->newpowermod = -1;
      fprintf (fd, "<P>Made the ship look less powerful\n");
      break;
    case MAGIC_BLESS_MEDICAL:
      player->magic_flags |= FLAG_BLESS_MEDICAL;
      fprintf (fd, "<P>The Merciful One watches over the away team\n");
      break;
    case MAGIC_SUMMON_POPCORN:
      player->popcorn++;
      fprintf (fd, "<P>A unit of popcorn appears, as if by magic\n");
      break;
    case MAGIC_STIMULATE:
      break;
    case MAGIC_SET_PROBE:
      if (player->star >= 0 && player->star < MAX_STAR)
        {
          player->probe = player->star;
          fprintf (fd, "<P>Probe deployed at %s\n", star_names[player->star]);
        }
      else
        player->favour[spells[spell].skill] += spells[spell].cost;
      break;
    case MAGIC_USE_PROBE:
      player->magic_flags |= FLAG_USE_PROBE;
      fprintf (fd, "<P>Probe report requested, see below for details\n");
      break;
    case MAGIC_CLEAR_PROBES:
      if (player->star < MAX_STAR)
        {
          for (p = 1; p < MAX_PLAYER; p++)
            if (players[p].probe == player->star)
              players[p].probe = NOWHERE;
          fprintf (fd, "<P>All probes cleared from %s\n",
                   star_names[player->star]);
        }
      break;
    case MAGIC_LUCKY_WARP:
    case MAGIC_LUCKY_IMPULSE:
    case MAGIC_LUCKY_SENSOR:
    case MAGIC_LUCKY_CLOAK:
    case MAGIC_LUCKY_SICKBAY:
    case MAGIC_LUCKY_LIFESUPPORT:
    case MAGIC_LUCKY_SHIELD:
    case MAGIC_LUCKY_WEAPON:
      item = lucky_item (player, spell - MAGIC_LUCKY_WARP);
      if (item == items)
        break;
      item->flags |= ITEM_LUCKY;
      fprintf (fd, "<P>Your %s feels lucky this turn\n", item_string (item));
      break;
    case MAGIC_SUPER_ENGINEERING:
    case MAGIC_SUPER_SCIENCE:
    case MAGIC_SUPER_MEDICAL:
    case MAGIC_SUPER_WEAPONRY:
      player->magic_flags |= (FLAG_SUPER_ENGINEERING <<
                              (spell - MAGIC_SUPER_ENGINEERING));
      fprintf (fd, "<P>%s crew burst into frantic energy\n",
               skill_names[spell - MAGIC_SUPER_ENGINEERING]);
      break;
    case MAGIC_ATONE_ENGINEERING:
    case MAGIC_ATONE_SCIENCE:
    case MAGIC_ATONE_MEDICAL:
    case MAGIC_ATONE_WEAPONRY:
      player->magic_flags |= (FLAG_ATONE_ENGINEERING <<
                              (spell - MAGIC_ATONE_ENGINEERING));
      fprintf (fd, "<P>Your %s officer atones to %s for your ships past misdeeds.\n",
               skill_names[spell - MAGIC_ATONE_ENGINEERING],
               god_names[spell - MAGIC_ATONE_ENGINEERING]);
      break;
    default:
      printf ("Bad spell cast (%d)\n", spell);
    }
}


void
check_favour (FILE * fd, struct PLAYER *player)
{
  skill_sort skill;
  int i, p, amount;
  struct ITEM *item;

  fprintf (fd, "<H3>Religious ");
  print_rules_link(fd, "Favour", "Favour");
  fprintf (fd, "</H3>\n");
  for (skill = engineering; skill <= weaponry; skill++)
    {
      if (player->rings_held & (0x10 << skill))
        {
          int done_some = FALSE;

          for (p = 0; p < MAX_PLAYER; p++)
            {
              if (player->rings_held & (0x10 << skill))
                if (p != player - players && players[p].star == player->star)
                  {
                    done_some |= give_favour (fd, skill, player, players + p);
                  }
            }
          if (done_some)
            fprintf (times, "<HR>%s giving out %s favour at %s\n",
                     name_string (player->name),
                     skill_names[skill], star_names[player->star]);
        }
      if (player->chosen & (1 << skill))
        {
          if (300 + dice(200) < player->evil)
            {
              fprintf (fd, "<BR>%s is displeased with you, and casts you out from their Chosen\n",
                       god_names[skill]);
              player->chosen &= ~(1 << skill);
              player->evil *= 3;
              player->evil /= 4;
            }
          else
            {
              fprintf (fd, "<BR>You are one of %s's Chosen\n", god_names[skill]);
              if (player->star == OLYMPUS)
                {
                  amount = dice (effective_skill_level (player, skill)) + 1;
                  fprintf (fd, " and gain %d extra favour at Olympus\n", amount);
                  player->favour[skill] += amount;
                }
            }
        }
      if (player->heretic & (1 << skill))
        {
          fprintf (fd,
                   "<BR>You are a heretic against the official prophet of %s\n",
                   god_names[skill]);
        }
      if (prophets[skill] == player - players)
        {
          int e = min (100, player->evil);
          int decay = fuzz ((100 + e) * player->favour[skill]) / 4000;
          if (e > 10)
            fprintf (fd,
                     "<BR>The %s is displeased with you, and you lose favour\n",
                     god_names[skill]);
          if (player->star != HOLIDAY && player->star < MAX_STAR)
            {
              int x;
              if (decay > 0)
                player->favour[skill] -= decay;

              x = fuzz(chosen[skill] * chosen[skill]);
              fprintf (fd,
                       "<BR>%s rewards your %d chosen ones at a cost to you of %d favour\n",
                       god_names[skill], chosen[skill], x);
              player->favour[skill] -= x;
              x = fuzz(old_favours[skill] / 15);
              x = max(0,x);
              player->favour[skill] += x;
              fprintf (fd,
                       "<BR>%s awards %d extra favour to you for the %s devotion of all starship captains\n",
                       god_names[skill], x, skill_names[skill]);
            }
          if (player->favour[skill] < 0)
            {
              fprintf (fd,
                       "<BR><EM>Sadly, your favour is still negative and you must retire as Prophet of %s</EM>\n",
                       god_names[skill]);
              retire_prophet (skill);
              fprintf (times, "<HR>%s retires as Prophet of %s\n",
                       name_string (player->name), god_names[skill]);
            }
        }
      else
        {
          if (player->star != HOLIDAY && player->star < MAX_STAR)
            {
              int decay = fuzz(player->favour[skill]) / 20;

              if (decay > 0)
                player->favour[skill] -= decay;
            }
        }
    }

  for (i = 0; i < 32; i++)
    {
      if ((player->experience[medical] & (1 << i)) && races[i].plague == 99)
        {
          fprintf (fd, "<P><EM>%s Plague is out of control at %s, ",
                   races[i].name, star_names[homeworlds[i]]);
          fprintf (fd,
                   "favour for your healing work there is withdrawn</EM>\n");
          player->experience[medical] &= ~(1 << i);
        }
    }


  if (player->star != HOLIDAY && player->star < MAX_STAR)
    {
      amount =
        min (power_rating (player),
             effective_skill_level (player, engineering));
      add_favour (player, engineering, amount);
      player->favour[engineering] += amount;

      amount =
        min (bitcount (player->experience[science]),
             effective_skill_level (player, science));
      add_favour (player, science, amount);
      player->favour[science] += amount;

      amount =
        min (bitcount (player->experience[medical]),
             effective_skill_level (player, medical));
      add_favour (player, medical, amount);
      player->favour[medical] += amount;

      amount =
        min (bitcount (player->enemies),
             effective_skill_level (player, weaponry));
      add_favour (player, weaponry, amount);
      player->favour[weaponry] += amount;

      item = items + player->ship;
      while (item != items)
        {
          if (item->sort == artifact)
            {
              for (i = 16; i < 24; i++)
                if (item->magic & (1 << i))
                  {
                    skill = (i - 16) / 2;
                    if (player->favour[skill] >= 5)
                      {
                        item->flags &= ~ITEM_BROKEN;
                        player->favour[skill] -= 5;
                      }
                    else
                      {
                        item->flags |= ITEM_BROKEN;
                        fprintf (fd,
                                 "<P><EM>You are unworthy to use the %s artifact</EM>\n",
                                 item_string (item));
                      }
                  }
            }
          item = items + item->link;
        }
    }


  for (skill = engineering; skill <= weaponry; skill++)
    fprintf (fd, "<LI>%s: %d\n", skill_names[skill], player->favour[skill]);

  if (player->star == popcorn.star && ! dybuk)
    {
      fprintf (fd,
               "<P>Chaos is chained here, and will richly reward the mortal who allows it to escape.\n");
      fprintf (fd,
               "<BR>%d popcorn goes to the first who releases Chaos to roam the galaxy again.\n",
               min (min (popcorn.impulse_limit, popcorn.sensor_limit),
                    popcorn.shield_limit));
    }

  if (player->tracer != 0)
    {
      if (players[player->tracer].star >= MAX_STAR)
        fprintf (fd, "<P>Tracer shows %s is on holiday\n",
                 name_string (players[player->tracer].name));
      else
        fprintf (fd, "<P>Tracer shows %s is at %s\n",
                 name_string (players[player->tracer].name),
                 star_names[players[player->tracer].star]);
    }

  if (player->reports)
    {
      char buf1[256];
      char buf2[256];

      snprintf (buf1, 256, "%s/results/%d/report_%d_%d.html",
                webroot, game, player - players, turn);
      snprintf (buf2, 256, "%s/Report_%d%s%d.htm",
                webroot, player - players,
                uint32_name (player->reports), turn);

      force_symlink (buf1,buf2);

      fprintf (fd,
               "<P><A HREF=\"http://%s/tbg/Report_%d%s%d.htm\">Starnet Terminal Report</A><p>",
               server, player - players, uint32_name (player->reports), turn);
    }
  if (player->viewing_trace && star_seen (player, players[player->tracer].star))
    {
      fprintf (fd, "<HR><H1>View from Tracer</H1>\n");
      show_starsystem (fd, player, players[player->tracer].star);
      fprintf (fd, "<HR>\n");
    }
  if (player->magic_flags & FLAG_USE_PROBE && star_seen (player, player->probe))
    {
      fprintf (fd, "<HR><H1>View from Probe</H1>\n");
      show_starsystem (fd, player, player->probe);
      fprintf (fd, "<HR>\n");
    }
}

void
commune (FILE * fd, struct PLAYER *player, skill_sort sort)
{
  fprintf (fd, "<P>%s officer communes with %s and gains %d favour\n",
           skill_names[sort], god_names[sort],
           effective_skill_level (player, sort));
  player->favour[sort] += effective_skill_level (player, sort);
}

void
choose (FILE * fd, struct PLAYER *player, int target, skill_sort sort)
{
  if (player - players != prophets[sort])
    {
      fprintf (fd,
               "<li>No choosing or unchoosing possible as you are not currently the Prophet of %s</li>\n",
               god_names[sort]);
      return;
    }
  if (players[target].chosen & (1 << sort))
    {
      players[target].chosen &= ~(1 << sort);
      fprintf (fd, "<li>%s struck off the list of %s's Chosen</li>\n",
               name_string (players[target].name), god_names[sort]);
    }
  else
    {
      players[target].chosen |= (1 << sort);
      fprintf (fd, "<li>%s added to the list of %s's Chosen</li>\n",
               name_string (players[target].name), god_names[sort]);
    }
}


void
write_heretic_lists ()
{
  int skill, p;

  fprintf (times, "<div class=\"heretics\">\n");
  for (skill = 0; skill < 4; skill++)
    {
      fprintf (times, "<hr>%s turns away from the heretics: ~\n",
               god_names[skill]);
      for (p = 1; p < MAX_PLAYER; p++)
        if (players[p].heretic & (1 << skill))
          fprintf (times, " %s ~", name_string (players[p].name));
    }
  fprintf (times, "</div>\n");
}

void
write_chosen_lists ()
{
  int skill, p;

  fprintf (times, "<div class=\"chosen\">\n");
  for (skill = 0; skill < 4; skill++)
    {
      fprintf (times, "<hr>%s recognises the chosen ones: ~\n",
               god_names[skill]);
      for (p = 1; p < MAX_PLAYER; p++)
        if (players[p].chosen & (1 << skill))
          fprintf (times, " %s ~", name_string (players[p].name));
    }
  fprintf (times, "</div>\n");
}

void
show_favour ()
{
  printf ("Numbers of chosen are: %d %d %d %d\n",
          chosen[engineering],
          chosen[science], chosen[medical], chosen[weaponry]);
  printf ("Average favours are: %d %d %d %d\n",
          favours[engineering] / 15,
          favours[science] / 15,
          favours[medical] / 15, favours[weaponry] / 15);
}

