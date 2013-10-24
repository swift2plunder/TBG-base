#include "skill.h"
#include "globals.h"
#include "util.h"
#include <stdio.h>
#include "rand.h"

int
enlightened (struct PLAYER *player, skill_sort sort)
{
  return (player->skills[sort] & 0x40);
}

/* counts the set bits in a skill bitmap */
int
skill_level (int skill)
{
  return (bitcount (skill));
}

int
skill_bit (skill_source sort, int number)
{
  switch (sort)
    {
    case academy_skill: /* 8..15 */
      return (0x100 << ((number - 1) & 7));
    case adventure_skill:       /* 16..31 */
      return (0x10000 << (number & 15));
    case school_skill:          /* 0..1 */
      return (0x1 << (number - 1));
    case repair_skill:          /* repair, 2..3 */
    case maintain_skill:        /* maintain, 4..5 */
      switch (number)
        {
        case warp_drive:
        case sensor:
        case life_support:
        case shield:
          return (sort == repair_skill ? 0x4 : 0x10);
        case impulse_drive:
        case cloak:
        case sick_bay:
        case ram:
        case gun:
        case disruptor:
        case laser:
        case missile:
        case drone:
        case fighter:
          return (sort == repair_skill ? 0x8 : 0x20);
        }
    case enlightenment_skill:
      return (0x40);
    case hacking_skill:
      return (0x80);
    default:
      printf ("Bad skill bit\n");
    }
  return (0);
}

void
show_skill (FILE * fd, skill_sort skill, int level)
{
  switch (level)
    {
    case 0:
    case 1:
      fprintf (fd, "<TD>School %d</TD>\n", level + 1);
      break;
    case 2:
    case 3:
      if (skill == weaponry && level == 3)
        fprintf (fd, "<TD>Repaired weapons</TD>\n");
      else
        fprintf (fd, "<TD>Repaired %s</TD>\n",
                 item_names[skill * 2 + level - 2]);
      break;
    case 4:
    case 5:
      if (skill == weaponry && level == 5)
        fprintf (fd, "<TD>Maintained weapons</TD>\n");
      else
        fprintf (fd, "<TD>Maintained %s</TD>\n",
                 item_names[skill * 2 + level - 4]);
      break;
    case 6:
      fprintf (fd, "<TD>Enlightenment</TD>\n");
      break;
    case 7:
      fprintf (fd, "<TD>Starnet Hacking</TD>\n");
      break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
      fprintf (fd, "<TD>Academy %d</TD>\n", level - 7);
      break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
      fprintf (fd, "<TD>%s</TD>\n", ad_types[level - 16].ad_name);
      break;
    }
}

int
effective_skill_level (struct PLAYER *player, skill_sort sort)
{
  int sk = (skill_level (player->skills[sort]) + isqrt (player->pools[sort]));
  if (player->chosen & (1 << sort))
    {
      sk += 16;
    }
  else if (player->magic_flags & (FLAG_SUPER_ENGINEERING << sort))
    {
      sk = rand_exp (32 + 3 * sk)/2;
    }
  return sk;
}

