#include "crew.h"
#include "rand.h"
#include "skill.h"
#include "util.h"
#include "items.h" /* for factor */
#include "tbg-big.h"

int
ground_combat (FILE * fd, struct PLAYER *player, int level, skill_sort sort,
               int medical_backup)
{
  int skill, risk;

  if (sort == weaponry)
    skill = effective_skill_level (player, weaponry);
  else
    skill = 0;
  if (player->artifacts & GROUND_COMBAT_BIT)
    skill = (skill * 3) / 2;

  if (player->crew[sort] == 0)
    {
      fprintf (fd, "<BR><EM>Landing party has no crew for combat</EM>\n");
      return (FALSE);
    }
  while (player->crew[sort])
    {
      risk = level - skill;
      if (risk < 0)
        risk = 0;
      fprintf (fd, "<BR>%s crew engage in ground combat at risk level %d%%\n",
               skill_names[sort], risk);
      if (dice (100) < risk)
        {
          fprintf (fd, "<BR><EM>One %s crew down - ", skill_names[sort]);
          if (medical_backup)
            {
              fprintf (fd, "Medical support team move in - ");
              if (dice (100) < effective_skill_level (player, medical))
                fprintf (fd, "and casualty is saved!");
              else
                {
                  fprintf (fd, "but it's no use!");
                  kill_crew (player, sort);
                }
            }
          else
            {
              fprintf (fd, "No Medical support on hand - injury is fatal");
              kill_crew (player, sort);
            }
          fprintf (fd, "</EM>\n");
          if (sort != weaponry)
            {
              fprintf (fd,
                       "<BR><EM>Non-weaponry crew retreat at first setback</EM>\n");
              return (FALSE);
            }
        }
      else
        {
          return (TRUE);
        }
    }
  fprintf (fd, "<BR><EM>All landing party crew dead</EM>\n");
  return (FALSE);
}

void
check_health (FILE * fd, struct PLAYER *p)
{
  skill_sort sort;
  int i;
  int all_crew = 0;
  
  if (p->star >= MAX_STAR)
    return;

  if (is_wraith (p))
    {
      for (sort = engineering; sort <= weaponry; sort++)
        if ((p->rings_held & (1 << sort)) &&
            p->crew[sort] < skill_level (p->skills[sort]))
          {
            fprintf (fd, "<P>A servant of chaos joins your %s crew\n",
                     skill_names[sort]);
            add_crew (p, sort, 1, skill_level (p->skills[sort]));
          }
    }
  for (sort = engineering; sort <= weaponry; sort++)
    {
      int total = p->crew[sort];        /* changed in kill_crew() */

      all_crew += total;

      for (i = 0; i < total; i++)
        {
          if (dice (1000) >= p->health)
            {
              fprintf (fd, "<BR><EM>One %s crew died</EM>\n",
                       skill_names[sort]);
              kill_crew (p, sort);
            }
        }
    }
  p->health -= rand_exp (all_crew/2);
  if (p->popcorn > 0)
    {
      p->health -= p->popcorn * 5;
      fprintf (fd, "<P>Popcorn cargo reduces health by %d%%\n", p->popcorn);
    }
  p->health += factor (life_support, p)/2;
  if (p->health > 999)
    p->health = 999;
  if (p->health < 1)
    p->health = 1;
}

int
total_pay (struct PLAYER *player)
{
  struct ITEM *item = items + player->ship;
  int result = 0;

  while (item != items)
    {
      if ((item->sort == pod) && (item->reliability >= BASE_UNIT))
        result += units[item->reliability - BASE_UNIT].pay;
      item = items + item->link;
    }
  if (player - players == ministers[PRESIDENT])
    {
      return result;
    }
  return (result +
          player->crew[engineering] + player->crew[science] +
          player->crew[medical] + player->crew[weaponry]);
}



void
kill_crew (struct PLAYER *player, skill_sort sort)
{
  player->pools[sort] -=
    (player->pools[sort] + player->crew[sort] - 1) / player->crew[sort];
  if (player->pools[sort] < 0)
    player->pools[sort] = 0;
  player->crew[sort]--;
}

void
add_crew (struct PLAYER *player, skill_sort skill, int number, int level)
{
  int total_health, total_crew = 0;
  skill_sort sk;

  for (sk = engineering; sk < weaponry; sk++)
    total_crew += player->crew[sk];
  total_health = total_crew * player->health + number * 990;

  total_crew += number;
  player->crew[skill] += number;
  player->pools[skill] += number * level;
  player->health = total_health / total_crew;
}

void
show_characters (FILE * fd, struct PLAYER *player)
{
  int i;
  skill_sort skill;
  int next[4];

  fprintf (fd, "<P><TABLE BORDER=1>\n");
  fprintf (fd, "<TR>");
  for (skill = engineering; skill <= weaponry; skill++)
    {
      fprintf (fd, "<TH>%s skills (%d = %d+%d)</TH>\n",
               skill_names[skill],
               effective_skill_level (player, skill),
               skill_level (player->skills[skill]),
               effective_skill_level (player, skill) -
               skill_level (player->skills[skill]));
      next[skill] = 0;
    }
  fprintf (fd, "</TR>");
  fprintf (fd, "<TR ALIGN=CENTER>");
  for (skill = engineering; skill <= weaponry; skill++)
    {
      fprintf (fd, "<TD>%d crew, average skill: %d</TD>\n",
               player->crew[skill],
               player->crew[skill] ?
               player->pools[skill] / player->crew[skill] : 0);
    }
  fprintf (fd, "</TR>");
  for (i = 0; i < 32; i++)
    {
      fprintf (fd, "<TR ALIGN=CENTER>");
      for (skill = engineering; skill <= weaponry; skill++)
        {
          while (!(player->skills[skill] & (1 << next[skill])) &&
                 (next[skill] < 32))
            next[skill]++;
          if (next[skill] < 32)
            {
              show_skill (fd, skill, next[skill]++);
            }
          else
            fprintf (fd, "<TD></TD>\n");
        }
      fprintf (fd, "</TR>");
      if ((next[engineering] & next[science] & next[medical] & next[weaponry])
          == 32)
        i = 32;
    }
  fprintf (fd, "</TABLE>\n");
}


void
recruit_rogues (FILE * fd, struct PLAYER *player, int loc)
{
  int number, level, current;
  skill_sort skill = (locations[loc].rogues >> 1) & 3;

  current = skill_level (player->skills[skill]);
  number = dice (current);
  if (ministers[MIN_JUST] == player - players)
    number = max(number, dice(current));
  number = min (number, current - player->crew[skill]);
  if (number == 0)
    {
      fprintf (fd, "<li>No rogues recruited.</li>\n");
      return;
    }

  level = dice (current - 1);
  if (ministers[MIN_JUST] == player - players)
    level = max(level, dice(current - 1));

  if (level > races[locations[loc].rogues >> 3].tech_level * 5)
    level = races[locations[loc].rogues >> 3].tech_level * 5;
  level++;
  while (level * number + player->pools[skill] > current * current)
    level--;
  fprintf (fd,
           "<li>Recruited %d rogues of average %s skill %d, you are declared an enemy of the %s government",
           number, skill_names[skill], level,
           races[locations[loc].rogues >> 3].name);
  if (ministers[MIN_JUST] == player - players)
    fprintf(fd, " and justice ministry files let you the pick the best of the rogues.");
  fprintf(fd, "</li>\n");
  add_crew (player, skill, number, level);
  player->enemies |= 1 << (locations[loc].rogues >> 3);
  locations[loc].rogues =
    (restock_rogues_race << 3) | (restock_rogues_skill << 1);
  printf ("New rogues: %x\n", locations[loc].rogues);
}

void
heal (FILE * fd, struct PLAYER *player)
{
  player->health += 4
    * factor (sick_bay, player);
  /*  if (player->health > 999)
      player->health = 999;*/
  fprintf (fd, "<li>%s officer healed crew's illness and injuries\n",
           skill_names[medical]);
  fprintf (fd, "Average health up to %d.%d%%</li>\n",
           player->health / 10, player->health % 10);
}

