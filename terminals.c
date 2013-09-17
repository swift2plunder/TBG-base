#include "globals.h"
#include "locations.h"

void
purge_accounts (struct PLAYER *player)
{
  int p;

  for (p = 0; p < num_players; p++)
    if (p != player - players)
      players[p].experience[science] &=
        ~(1 << star_has_loc (player->star, terminal));
}

