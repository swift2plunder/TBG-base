#include <stdlib.h>
#include <string.h>
#include "globals.h"
#include "tbg.h"
#include "data.h"
#include "opts.h"

void
remind (int turn, int game)
{
  FILE *fd, *temp;
  char buffer[256];
  int p;

  sprintf (buffer, "%s/remind", desired_directory);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create reminder script\n");
      exit (1);
    }
  for (p = 1; p < MAX_PLAYER; p++)
    {
      char *address = players[p].address;
      if (!strcmp (address, "nobody@localhost"))
        continue;
      if (players[p].preferences & 32)
        continue;
      sprintf (buffer, "%s/orders/%d/%s%d",
               webroot, game, players[p].name, turn);
      temp = fopen (buffer, "r");
      if (temp)
        {
          fclose (temp);
          continue;
        }
      //fprintf (fd, "sleep 1\n");
      fprintf (fd, "mail -s \"TBG Reminder - %s\" %s <%s/remind\n",
               players[p].name, address, gameroot);
    }
  fclose (fd);
  sprintf (buffer, "/bin/sh %s/remind", desired_directory);
  system (buffer);
  //printf ("%s\n", buffer);
}

int
main (int agrc, char *argv[])
{
  gameroot="/home/tbg/work";
  desired_directory="/home/tbg/work/tbg";
  webroot="/home/tbg/work/WWW";

  game = 1;
  turn = -1;
  read_master_file ();
  read_data ();
  read_players ();
  remind(turn, game);

  exit(0);
}

