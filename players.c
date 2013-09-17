#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "globals.h"
#include "tbg.h"
#include "rand.h"

#define ERROR( func ) do { perror( func ); fprintf (stderr, "ERROR on %s:%d", __FILE__, __LINE__ );  } while (0)



int
backup_and_save (char *ofname, char *fbuf)
{
  int ofnamechars = strlen(ofname);
  char newfilename[2048];
  char ebuf[8192];
  struct tm tm_now;
  time_t time_now = time(0);
  struct tm *tm_ret;
  int ofd;
  FILE *ofile;
  int retval;

  ebuf[0] = 0;
  
  tm_ret = localtime_r (&time_now, &tm_now);
  if (! tm_ret)
    {
      ERROR ("localtime_r in backup_and_save");
      return -1;
    }

  sprintf (newfilename, "%s", ofname);
  retval = strftime (newfilename + ofnamechars,
                     sizeof (newfilename) - ofnamechars,
                     "-%F-%T-XXXXXX", &tm_now);
  if (retval == 0)
    {
      ERROR ("strftime in backup_and_save");
      return -1;
    }
  ofd = mkstemp (newfilename);
  if (ofd == -1)
    {
      ERROR ("mkstemp in backup_and_save");
      return -1;
    }
  ofile = fdopen (ofd, "w");
  if (! ofile)
    {
      ERROR ("fdopen in backup_and_save");
      return -1;
    }
  retval = fprintf (ofile, "%s", fbuf);
  if (retval < 0)
    {
      ERROR ("fprintf in backup_and_save");
      return -1;
    }
  fclose (ofile);
  retval = chmod (newfilename, 0770);
  if (retval < 0)
    {
      ERROR ("(non-fatal) chmod in backup_and_save");
    }
  chown (newfilename, 33, 119);
  if (retval < 0)
    {
      ERROR ("(non-fatal) chown in backup_and_save");
    }
  if (unlink(ofname))
    {
      if (errno != ENOENT)
        {
          ERROR ("unlink in backup_and_save");
          return -1;
        }
    }
  if (symlink(newfilename, ofname))
    {
      ERROR ("symlink in backup_and_save");
      return -1;
    }
  return 0;
}


void
read_players ()
{
  FILE *fd;
  int dummy, account;
  int player = 0, pref;
  char name[128], address[128], realname[80];
/*  
  sprintf (name, "%s/players", gameroot);
  fd = fopen (name, "r");
  if (!fd)
    {
      printf ("Can't open players file %s for reading in gameroot.\n", name);
      exit (1);
    }
  for (player = 0; player < MAX_ACCOUNT; player++)
    {
      fscanf (fd, "%d %s %d %s\n", &dummy, address, &dummy,
              passwords[player] = malloc (128));
      fgets (realname, 75, fd);
    }
  fclose (fd);
*/
  sprintf (name, "%s/players", desired_directory);
  fd = fopen (name, "r");
  if (!fd)
    {
      printf ("Can't open players file %s for reading in desired directory.\n", name);
      exit (1);
    }
  for (player = 0; player < MAX_PLAYER; player++)
    {
      fscanf (fd, "%s %d %d %d %s\n", name, &account, &pref, &dummy,
              players[player].address);
      players[player].account_number = account;
      strcpy (players[player].name, name);
      players[player].preferences = pref;
    }
  fclose (fd);
  num_players = MAX_PLAYER;
}

void
write_players ()
{
  int player = 0;
  char name[128];
  
  char buf[65536];
  int chars = 0;

  buf[0] = 0;
  
  for (player = 0; player < MAX_PLAYER; player++)
    {
      size_t size =  sizeof(buf) - chars - 1;
      size_t ret;
      ret = snprintf (buf +chars, size, "%s %d %d %d %s\n",
                      players[player].name,
                      players[player].account_number,
                      players[player].preferences, 0,
                      players[player].address);
      if (ret > size)
        exit(-1);
      chars += ret;
    }
  sprintf (name, "%s/players", desired_directory);
  backup_and_save (name,buf);
}
/*
void
read_master_file ()
{
  FILE *fd;
  int normal_turn;
  char name[128];

  sprintf (name, "%s/g%d", desired_directory, game);
  fd = fopen (name, "r");
  if (!fd)
    {
      printf ("Can't open g%d file: %s\n", game, name);
      exit (1);
    }
  fscanf (fd, "%d\n", &normal_turn);
  fscanf (fd, "%d\n", &seed);
  if (turn == -1)
    turn = normal_turn;       
  if (turn < normal_turn)
    {
      printf ("Can't run a turn earlier than %d!\n", normal_turn);
      exit (1);
    }
  fclose (fd);
  init_rng();
}
*/

void
read_master_file ()
{
  FILE *fd;
  int dummy, account;
  int player = 0, normal_turn, pref;
  char name[128], address[128], realname[80];
  char password[128];
/*
  sprintf (name, "%s/players", desired_directory);
  fd = fopen (name, "r");
  if (!fd)
    {
      printf ("Can't open players file %s\n", name);
      exit (1);
    }
  for (player = 0; player < MAX_ACCOUNT; player++)
    {
      fscanf (fd, "%d %s %d %s\n", &dummy, address, &dummy,
              passwords[player] = malloc (128));
      fgets (realname, 75, fd);
      if (player < MAX_PLAYER)
        strcpy (players[player].address, address);
    }
  fclose (fd);
*/
  sprintf (name, "%s/g%d", desired_directory, game);
  fd = fopen (name, "r");
  if (!fd)
    {
      printf ("Can't open g%d file\n", game);
      exit (1);
    }
  fscanf (fd, "%d\n", &normal_turn);
  fscanf (fd, "%d\n", &seed);
  if (turn == -1)
    turn = normal_turn;         /* not overriden for testing */
  if (turn < normal_turn)
    {
      printf ("Can't run a turn earlier than %d!\n", normal_turn);
      exit (1);
    }
  srand (turn + seed);
  for (player = 0; player < MAX_PLAYER; player++)
    {
      fscanf (fd, "%s %d %d %d\n", name, &account, &pref, &dummy);
      players[player].account_number = account;
      strcpy (players[player].name, name);
      players[player].preferences = pref;
    }
  fclose (fd);
  num_players = MAX_PLAYER;
}

void
write_master_file ()
{
  FILE *fd;
  int player = 0;
  char name[128];
  char buffer[128];

  sprintf (name, "%s/g%d", desired_directory, game);
  sprintf (buffer, "%s-%d.bak", name, turn);
  link (name, buffer);
  unlink (name);
  fd = fopen (name, "w");
  if (!fd)
    {
      printf ("Can't create new master file\n");
      exit (1);
    }
  fprintf (fd, "%d\n%d\n", turn + (really_send ? 1 : 0), seed);
  for (player = 0; player < MAX_PLAYER; player++)
    {
      if (really_send)
        players[player].preferences &= ~32;
      /* remove restart command */
      fprintf (fd, "%s %d %d 0\n",
               players[player].name,
               players[player].account_number, players[player].preferences);
    }
  fclose (fd);
}

