#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "bytes.h"

#define FALSE   0
#define TRUE ~FALSE

#define OLD_MAX_ACCOUNT 600
#define MAX_ACCOUNT     600
#define MAILING_LIST    999
#define GET_RULES       1000
#define GET_TIMES       1001

#define max_player      200

int turns_left[MAX_ACCOUNT];
int free_turns[MAX_ACCOUNT];
int accounts[MAX_ACCOUNT];
char *addresses[MAX_ACCOUNT];
char *names[MAX_ACCOUNT];
char *realnames[MAX_ACCOUNT];
int preferences[MAX_ACCOUNT];
char *passwords[MAX_ACCOUNT];
char *secrets[MAX_ACCOUNT];

int seed, last_turn = 0;

char *mailbox;

#define TATA(game)      (game >= 3)
#define SECURE(game)    (game >= 4)

void
format (FILE * fd, char *buffer)
{
  char ch;
  int dodgy_ms = (int) strstr (buffer, "z=3D");

  while (*buffer)
    {
      switch (*buffer)
        {
        case '+':
          fprintf (fd, " ");
          buffer++;
          break;
        case '%':
          ch = hex_to_char (buffer + 1);
          fprintf (fd, "%c", ch);
          buffer += 3;
          break;
        case '&':
          fprintf (fd, "\n");
          buffer++;
          break;
        case '\n':
          buffer++;
          break;
        case '=':
          if (dodgy_ms)
            {
              if (buffer[1] == '\n')
                {
                  buffer += 2;
                  break;
                }
              ch = hex_to_char (buffer + 1);
              fprintf (fd, "%c", ch);
              buffer += 3;
              break;
            }
        default:
          if (*buffer < 32)
            {
              fprintf (fd, " ");
              buffer++;
            }
          else
            fprintf (fd, "%c", *buffer++);
          break;
        }
    }
  fprintf (fd, "\n");
}

void
read_addresses (int game)
{
  FILE *fd;
  char buffer[80], *p;
  int i, n, check;

  game = 1;
  sprintf (buffer, "%s/players", getenv ("HOME"));
  fd = fopen (buffer, "r");
  if (!fd)
    {
      printf ("Can't open players file %s\n", buffer);
      exit (1);
    }
  for (n = 0; n < OLD_MAX_ACCOUNT; n++)
    {
      secrets[n] = NULL;
      fscanf (fd, "%d %s %d %s ",
              &check,
              p = malloc (80), turns_left + n, passwords[n] = malloc (80));
      addresses[n] = p;
      fgets (realnames[n] = malloc (80), 75, fd);
      realnames[n][strlen (realnames[n]) - 1] = '\0';
      if (realnames[n][0] == ' ')
        realnames[n]++;
      if (check != n)
        {
          printf ("Player file corrupt at %d != %d!\n", n, check);
          exit (1);
        }
    }
  fclose (fd);

  while (n < MAX_ACCOUNT)
    {
      secrets[n] = NULL;
      turns_left[n] = 0;
      addresses[n] = "nobody@localhost";
      realnames[n] = "Fakename";
      passwords[n] = "x";
      n++;
    }
  sprintf (buffer, "%s/tbg/g%d", getenv ("TBG"), game);
  fd = fopen (buffer, "r");
  if (!fd)
    {
      printf ("Can't open game file %s\n", buffer);
      exit (1);
    }
  fscanf (fd, "%d\n%d\n", &last_turn, &seed);
  last_turn--;
  /* read/discard first two numbers */

  for (n = 0; n < max_player; n++)
    {

      fscanf (fd, "%s %d %d %d\n",
              names[n] = malloc (32),
              accounts + n, preferences + n, free_turns + n);
      if (accounts[n] && accounts[n] < 200)
        /* might be a duplicate */
        {
          for (i = 200; i < MAX_ACCOUNT; i++)
            if (strcasecmp (addresses[i], addresses[accounts[n]]) == 0)
              accounts[n] = i;
        }
    }
  fclose (fd);
}

/* returns true if more to come */
int
parse_header (FILE * fd, char *buffer, char *from, char *reply, char *subject,
              char *id)
{
  *from = '\0';                 /* just in case */
  do
    {
      if (buffer[0] == '\n' && !feof (fd))
        return (TRUE);
      if (strncmp (buffer, "X-Real-Host-From:", 17) == 0)
        {
          strcpy (from, buffer + 18);
          from[strlen (from) - 1] = '\0';
        }
      if (strncmp (buffer, "From:", 5) == 0)
        {
          buffer[strlen (buffer) - 1] = '\0';
          if (strchr (buffer, '<'))
            {
              strcpy (reply, strchr (buffer, '<') + 1);
              if (strchr (reply, '>'))
                *strchr (reply, '>') = '\0';
              *strchr (buffer, '<') = '\0';
            }
          else
            {
              strcpy (reply, buffer + 6);
              if (strchr (reply, ' '))
                *strchr (reply, ' ') = '\0';
              if (strchr (buffer, '('))
                *strchr (buffer, '(') = '\0';
            }
          if (strchr (buffer, '"'))
            {
              strcpy (from, strchr (buffer, '"') + 1);
              if (strchr (from, '"') != 0)
                *strchr (from, '"') = '\0';
            }
          else
            strcpy (from, buffer + 6);
        }
      if (strncmp (buffer, "Subject:", 8) == 0)
        {
          char *p = buffer + 8;

          while (*p == ' ')
            p++;
          strcpy (subject, p);
          subject[strlen (subject) - 1] = '\0';
        }
      if (strncmp (buffer, "Message-I", 9) == 0)
        {
          char *p = id, *q = buffer + 11;
          while (*q)
            {
              switch (*q)
                {
                case '>':
                case '<':
                case ' ':
                case '\n':
                  break;
                default:
                  *p++ = *q;
                  break;
                }
              q++;
            }
          *p = '\0';
        }
    }
  while (fgets (buffer, 1024, fd));
  return (FALSE);
}

char *
munge_password (char *password)
{
  static char buffer[256];
  char *p = password, *q = buffer;

  while (*p)
    {
      if (isalnum (*p))
        *q++ = *p;
      p++;
    }
  *q == '\0';
  if (buffer[0] == '\0')
    return ("x");
  else
    return (buffer);
}

char *
munge_name (char *name)
{
  while (strchr (name, ' '))
    *strchr (name, ' ') = '_';
  while (strchr (name, '@'))
    *strchr (name, '@') = '_';
  while (name[strlen (name) - 1] == '_')
    name[strlen (name) - 1] = '\0';
  return (name);
}

int
check_name (char *name)
{
  int ok;

  if (!strchr (name, '@'))
    return (FALSE);
  if (strchr (name, '\''))
    return (FALSE);
  if (strchr (name, ' '))
    return (FALSE);
  if (strlen (name) < 3)
    return (FALSE);
  while (*name)
    {
      ok = FALSE;
      if (*name >= 'a' && *name <= 'z')
        ok++;
      if (*name >= 'A' && *name <= 'Z')
        ok++;
      if (*name >= '0' && *name <= '9')
        ok++;
      switch (*name)
        {
        case '@':
        case '_':
        case '-':
        case '.':
        case '+':
          ok++;
        }
      if (!ok)
        return (FALSE);
      name++;
    }
  return (TRUE);
}

int
check_msg_done (char *id)
{
  return (FALSE);
}

void
remind (int turn, int game)
{
  FILE *fd, *temp;
  char buffer[256];
  int p;

  sprintf (buffer, "%s/tbg/remind", getenv ("TBG"));
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create reminder script\n");
      exit (1);
    }
  for (p = 1; p < max_player; p++)
    {
      char *address = addresses[p];
      if (!strcmp (address, "nobody@localhost"))
        continue;
      if (preferences[p] & 32)
        continue;
      sprintf (buffer, "%s/orders/%d/%s%d",
               getenv ("TBG"), game, names[p], turn);
      temp = fopen (buffer, "r");
      if (temp)
        {
          fclose (temp);
          continue;
        }
      fprintf (fd, "sleep 1\n");
      fprintf (fd, "mail -s \"TBG Reminder - %s\" %s -- -f tbg-moderator@asciiking.com <%s/remind\n",
               names[p], address, getenv ("TBG"));
    }
  fclose (fd);
  sprintf (buffer, "/bin/sh %s/tbg/remind", getenv ("TBG"));
  system (buffer);
}


void
new_player (int game, int player, int turn, char *address, char *name)
{
  FILE *fd;
  char buffer[256];
  int p;

  if (strchr (name, ' ') || !name[0] || !address[0])
    {
      printf ("Tried to add ship with a bad name or address (%s) (%s)!\n",
              name, address);
      sprintf (buffer,
               "mail -s \"TBG: Bad Shipname %s\" tbg-moderator@asciiking.com <%s/spacename",
               name, getenv ("TBG"));
      system (buffer);
      return;
    }
  for (p = 0; p < max_player; p++)
    if (strcmp (name, names[p]) == 0)
      {
        printf ("Tried to add duplicate ship %s!\n", name);
        sprintf (buffer,
                 "mail -s \"TBG: Bad Shipname %s\" tbg-moderator@asciiking.com <%s/duplicate",
                 name, getenv ("TBG"));
        system (buffer);
        return;
      }
  if (player == 0)              /* auto select */
    {
      player = 1;
      while (player < max_player && accounts[player])
        player++;
      if (player == max_player)
        {
          printf ("Can't add %s (%s) - no room\n", name, address);
          return;
        }
    }
  printf ("Added %s to replace %s\n", name, names[player]);
  accounts[player] = player;
  sprintf (buffer, "%s/orders/%d/%s%d", getenv ("TBG"), game, name, turn);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create startup orders file\n");
      exit (1);
    }
  fprintf (fd, "Z=%d\n", game);
  fprintf (fd, "z=%d\n", turn);
  fprintf (fd, "n=%s\n", name);
  fprintf (fd, "y=1\n");
  fclose (fd);
  strcpy (names[player], name);
  strcpy (addresses[player], address);
}

/* creates new account for player with name and address */
int
new_account (int player, char *address, char *name)
{
  char buffer[256];
  int p;

  if (player == 0)              /* auto select */
    {
      player = 200;
      while (player < MAX_ACCOUNT && strcmp (addresses[player], "nobody@localhost"))
        player++;
      if (player == MAX_ACCOUNT)
        {
          printf ("Can't add %s (%s) - no room\n", name, address);
          return;
        }
    }
  printf ("Added %s to replace %s (%d)\n", name, addresses[player], player);
  strcpy (realnames[player], name);
  strcpy (addresses[player], address);
  passwords[player] = munge_password (realnames[player]);
  sprintf (buffer,
           "echo \"%s,\nYour w++ account (number %d) has been set up.\nYour secret URL is http://tbg.asciiking.com/tbg/players/%s_%d.htm\" | mail -s \"w++ account opened\" %s",
           realnames[player], player, passwords[player], player,
           addresses[player]);
  system (buffer);
  return (player);
}

void
charge (int game)
{
  int p, account;

  for (p = 1; p < max_player; p++)
    {
      account = accounts[p];
      turns_left[account]--;
      if (turns_left[account] <= 0)
        {
          printf ("Charge %s something\n", addresses[account]);
          turns_left[account] += 60;
        }
    }
}

void
update_files (int game)
{
  FILE *fd;
  char name[256];
  int p;

  sprintf (name, "%s/players", getenv ("TBG"));
  fd = fopen (name, "w");
  if (!fd)
    {
      printf ("Can't create master player file!\n");
      exit (1);
    }
  for (p = 0; p < MAX_ACCOUNT; p++)
    fprintf (fd, "%d %s %d %s %s\n", p, addresses[p],
             turns_left[p], passwords[p], realnames[p]);
  fclose (fd);
  sprintf (name, "%s/tbg/g%d", getenv ("TBG"), game);
  fd = fopen (name, "w");
  if (!fd)
    {
      printf ("Can't create new game file!\n");
      exit (1);
    }
  fprintf (fd, "%d\n%d\n", last_turn + 1, seed);
  for (p = 0; p < max_player; p++)
    fprintf (fd, "%s %d %d %d\n",
             names[p], accounts[p], preferences[p], free_turns[p]);
  fclose (fd);
}


void
read_secrets ()
{
  FILE *fd;
  char buffer[256];
  int a, p;

  sprintf (buffer, "%s/tbg1_secrets", getenv ("TBG"));
  fd = fopen (buffer, "r");
  if (!fd)
    {
      printf ("Can't open secrets list\n");
      exit (1);
    }
  for (p = 0; p < max_player; p++)
    {
      fscanf (fd, "%d %s\n", &a, buffer);
      secrets[a] = malloc (32);
      strcpy (secrets[a], buffer);
    }
  fclose (fd);
}

void
make_player_page (int player)
{
  FILE *fd, *user_page;
  char buffer[256];

  read_secrets ();
  sprintf (buffer, "%s/WWW/players/%s_%d.htm",
           getenv ("TBG"), passwords[player], player);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create player file %s!\n", buffer);
      exit (1);
    }
  fprintf (fd, "<H1>Player page for %s (number %d)</H1>\n",
           realnames[player], player);
  fprintf (fd, "Here's your custom <A HREF=\"%d.html\">web page</A>,\n",
           player);
  if (secrets[player])
    {
      fprintf (fd,
               " your <A HREF=\"http://tbg.asciiking.com/tbg/%s.htm\">TBG-1 results</A>,\n",
               secrets[player]);
      fprintf (fd,
               " access to <A HREF=\"http://tbg.asciiking.com/turnulator2/\">Corby's Turnulator</A>,\n");
    }
  fprintf (fd, " and your current credit is %d<hr>\n", turns_left[player]);
  fprintf (fd, "To change password: \n");
  fprintf (fd,
           "<form action=\"http://tbg.asciiking.com/tbg/tbgmail.cgi\" method=post>\n");
  fprintf (fd, "<input type=hidden name=\"Z\" value=1>\n");
  fprintf (fd, "<input type=hidden name=\"z\" value=-18>\n");
  fprintf (fd, "<input type=hidden name=\"t\" value=%d>\n", player);
  fprintf (fd, "<input type=text name=\"s\" value=%s>\n", passwords[player]);
  fprintf (fd, "<input type=submit value=\"Confirm Change\">\n");
  fprintf (fd, "</form><hr>\n");

  fprintf (fd,
           "To upload an image to <a href=\"http://tbg.asciiking.com/tbg/images\">TBG images</a>:\n");
  fprintf (fd,
           "<form action=\"http://tbg.asciiking.com/tbg/tbgmail.cgi\" method=post>\n");
  fprintf (fd, "<input type=hidden name=\"Z\" value=1>\n");
  fprintf (fd, "<input type=hidden name=\"z\" value=-19>\n");
  fprintf (fd, "<input type=hidden name=\"t\" value=%d>\n", player);
  fprintf (fd,
           "Server (eg www.geocities.com): <input type=text name=\"s\">\n");
  fprintf (fd,
           "<br>Image (eg /biz/pics/duck.gif): <input type=text name=\"n\">\n");
  fprintf (fd, "<br><input type=submit value=\"Upload Now\">\n");

  fprintf (fd, "</form><hr>\n");

  fprintf (fd, "Your page source for editting:<br>\n");
  fprintf (fd,
           "<form action=\"http://tbg.asciiking.com/tbg/tbgmail.cgi\" method=post>\n");
  fprintf (fd, "<input type=hidden name=\"Z\" value=1>\n");
  fprintf (fd, "<input type=hidden name=\"z\" value=-15>\n");
  fprintf (fd, "<input type=hidden name=\"c\" value=\"E\">\n");
  fprintf (fd, "<input type=hidden name=\"t\" value=%d>\n", player);
  fprintf (fd, "<textarea cols=75 rows=40 name=\"n\">\n");
  sprintf (buffer, "%s/WWW/players/%d.html", getenv ("TBG"), player);
  user_page = fopen (buffer, "r");
  if (user_page)
    {
      while (fgets (buffer, 250, user_page))
        fprintf (fd, "%s", buffer);
      fclose (user_page);
    }
  fprintf (fd, "</textarea>\n");
  fprintf (fd, "<input type=submit value=\"Confirm Edit\">\n");
  fprintf (fd, "</form>\n");
  fclose (fd);
}

char *
get_title (int player)
{
  FILE *fd;
  char buffer[256];
  static char result[1024];
  char *p;

  strcpy (result, "No Title");

  sprintf (buffer, "%s/WWW/players/%d.html", getenv ("TBG"), player);
  fd = fopen (buffer, "r");
  if (!fd)
    return ("Error!");
  while (fgets (buffer, 250, fd))
    {
      p = strstr (buffer, "<title>");
      if (!p)
        p = strstr (buffer, "<TITLE>");
      if (p)
        {
          strcpy (result, p + 7);
          p = strchr (result, '<');
          if (p)
            *p = '\0';
        }
    }
  fclose (fd);
  return (result);
}

void
make_index_page ()
{
  int p;
  char buffer[256];
  FILE *fd;                     /* index page */
  FILE *user_page;
  int size;
  struct stat status;
  struct tm *mod_time;
  time_t now;
  double interval;

  now = time (NULL);
  sprintf (buffer, "%s/WWW/players/index.html", getenv ("TBG"));
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create player index page!\n");
      exit (1);
    }
  fprintf (fd, "<title>w++ Player Pages</title>\n");
  fprintf (fd, "<center><h1>w++ Player Pages</h1>\n");

  fprintf (fd, "<h2><a href=\"/tbg/news/news.cgi\">New Newsgroup</a></h2>");

  fprintf (fd, "(modified means changed in the last week)<br>\n");
  fprintf (fd, "<table border=1>\n");
  fprintf (fd, "<th>Title</th><th>Size in bytes</th><th>Last Changed</th>\n");
  for (p = 200; p < MAX_ACCOUNT; p++)
    {
      sprintf (buffer, "%s/WWW/players/%d.html", getenv ("TBG"), p);
      user_page = fopen (buffer, "r");
      if (user_page)
        {
          stat (buffer, &status);
          mod_time = localtime (&(status.st_mtime));
          close (user_page);
          size = status.st_size;
          if (!size)
            continue;
          interval = difftime (now, status.st_mtime);
        }
      else
        continue;
      fprintf (fd, "<tr align=center>\n");
      fprintf (fd, "<td><a href=\"%d.html\">%s</a> %s</td>",
               p,
               get_title (p),
               interval < 60.0 * 60.0 * 24.0 * 7.0 ? "(modified)" : "");
      fprintf (fd, "<td>%d</td>", size);
      fprintf (fd, "<td>%s</td></tr>\n", asctime (mod_time));
      /* what's the next bit for? */
      sprintf (buffer, "rm -f $TBG/WWW/players/%s_%d.htm", passwords[p], p);
      system (buffer);
      make_player_page (p);
    }
  fprintf (fd, "</table></center>\n");
  fclose (fd);
}


void
update_player_page (int target, char *text)
{
  FILE *fd;
  char buffer[256];

  sprintf (buffer, "%s/WWW/players/%d.html", getenv ("TBG"), target);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      printf ("Can't create user page %s!\n", buffer);
      exit (1);
    }
  fprintf (fd, text);
  fclose (fd);
  make_index_page ();
}

int
check_key (char *name, int turn, int key)
{
  int checksum;
  char *p = name;

  checksum = make_key(name,turn);
  if (checksum != key)
    {
      printf ("Still wrong key for %s\n", name);
      /* forged orders ! */
      name[0] = '_';
      /* stop it being acceptable */
      return (FALSE);
    }
  return (TRUE);
}

int
bad_word (char *p)
{
  while (*p)
    {
      char c = *p++;
      if (c >= 'A' && c <= 'Z')
        continue;
      if (c >= 'a' && c <= 'z')
        continue;
      if (c >= '0' && c <= '9')
        continue;
      switch (c)
        {
        case '/':
        case '.':
        case '-':
        case '_':
        case '~':
          break;
        default:
          return (TRUE);
        }
    }
  return (FALSE);
}

void
copy_message (FILE * fd, char *id, char *from)
{
  FILE *tmp;
  char buffer[65000], reply[132], subject[132];
  char tempfilename[100];
  int turn = -10, target = 0, game = -1;
  int key = 0;
  char command = '@';
  char name[65000], shipname[256];
  char *p;
  int valid_orders = FALSE;

  name[0] = '\0';
  shipname[0] = '\0';
  strncpy(tempfilename, "tbgtemp-XXXXXX", 100);
  mkstemp(tempfilename);
  tmp = fopen (tempfilename, "w");
  if (!tmp)
    {
      printf ("Can't create new file\n");
      exit (1);
    }
  fprintf (tmp, "#=%s\n", from);
  while (fgets (buffer, 1000, fd))
    {
      if (strncmp (buffer, "n=", 2) == 0)
        {
          strcat (name, buffer + 2);
          do
            {
              if (!fgets (buffer, 1000, fd))
                sprintf (buffer, " =");
              if (buffer[1] != '=')
                {
                  strcat (name, buffer);
                }
            }
          while ((buffer[0] == '\n') || (buffer[1] && buffer[1] != '='));
          if (name[strlen (name) - 1] == '\n')
            name[strlen (name) - 1] = '\0';
          if (!TATA (game) && command != 'E')
            while (p = strchr (name, '\"'))
              *p = '^';
        }
      if (strncmp (buffer, "c=", 2) == 0)
        command = buffer[2];
      if (strncmp (buffer, "k=", 2) == 0)
        key = atoi (buffer + 2);
      if (strncmp (buffer, "Z=", 2) == 0)
        game = atoi (buffer + 2);
      if (strncmp (buffer, "z=", 2) == 0)
        turn = atoi (buffer + 2);
      if (strncmp (buffer, "t=", 2) == 0)
        {
          char *p = buffer + 2;

          while (*p)
            {
              target += atoi (p);
              while (*p >= '0' && *p <= '9')
                p++;
              while (*p && (*p < '0' || *p > '9'))
                p++;
            }
        }
      if (strncmp (buffer, "s=", 2) == 0)
        {
          strcpy (shipname, buffer + 2);
          shipname[strlen (shipname) - 1] = '\0';
        }
      if (strncmp (buffer, "From ", 5) == 0)
        break;
      fprintf (tmp, "%s", buffer);
    }
  fclose (tmp);

  if (game == -1)
    {
      if (turn != -11 && turn != -10 && turn != -17)
        printf ("Game still -1 for command %d\n", turn);
      game = 1;
    }
  read_addresses (game);

  switch (turn)
    {
    case -19:                   /* upload image */
      sprintf (buffer, "$TBG/bin/getimage %s %s %d", shipname, name, target);
      if (bad_word (shipname) || bad_word (name))
        {
          printf ("Bad command\n%s\n", buffer);
          break;
        }
      printf ("Running %s\n", buffer);
      system (buffer);
      break;
    case -18:                   /* change password */
      sprintf (buffer, "rm $TBG/WWW/players/%s_%d.htm",
               passwords[target], target);
      system (buffer);
      passwords[target] = munge_password (shipname);
      make_player_page (target);
      break;
    case -17:                   /* account application */
      if (strlen (name) < 3 || strlen (shipname) < 1 || (!check_name (name)))
        {
          sprintf (buffer,
                   "echo \"Dumb account opening (%s, %s) at `date` \" >> $TBG/log",
                   name, shipname);
          system (buffer);
          break;
        }
      sprintf (buffer, "mail -s \"w++ Account Opening\" %s -- -f tbg-moderator@asciiking.com <$TBG/welcome",
               name);
      system (buffer);
      sprintf (buffer,
               "echo \"\t%s %s (%s)\nt=0\nz=-16\nZ=1\ns=%s\nn=%s\n\" | mail -s \"TBG: Open %s (%s)\" tbg-moderator@asciiking.com",
               shipname, name, from, shipname, name, shipname, name);
      system (buffer);
      break;
    case -16:
      target = new_account (target, name, shipname);
      make_player_page (target);        /* not 0 ! */
      break;
    case -15:
      update_player_page (target, name);
      break;
    case -14:
      charge (game);
      break;
    case -13:
      new_player (game, target, last_turn + 1, name, shipname);
      break;
    case -12:                   /* old reminder */
      break;
    case -22:
      remind (last_turn + 1, game);
      break;
    case -11:                   /* TBG startup application */
      if (strlen (name) < 3 || strlen (shipname) < 1 || (!check_name (name)))
        {
          sprintf (buffer,
                   "echo \"Dumb startup (%s, %s) at `date` \" >> $TBG/log",
                   name, shipname);
          system (buffer);
          break;
        }
      if (target)
        sprintf (buffer,
                 "mail -s \"TBG Rejection\" %s <$TBG/rejection",
                 name);
      else
        {
          sprintf (buffer, "mail -s \"TBG Acceptance\" %s <$TBG/acceptance",
                   name);
          system (buffer);
          sprintf (buffer,
                   "echo \"\t%s %s (%s)\nt=0\nz=-13\nZ=1\ns=%s\nn=%s\n\" | mail -s \"TBG: New %s (%s)\" tbg-moderator@asciiking.com",
                   shipname, name, from, shipname, name, shipname, name);
        }
      system (buffer);
      break;
    case -10:
      sprintf (buffer, "cat %s >>$TBG/Mail/misc", mailbox);
      system (buffer);
      break;
    case -1:
      printf ("Bad old request for rules\n");
      break;
    case -2:                    /* not a turn but an internal mailing */
      if (check_msg_done (id))
        return;
      if (target < 0)
        return;
      if (target >= GET_RULES && (!check_name (name)))
        {
          if (strlen (name) > 1)
            printf ("Dodgy name <<<%s>>>!\n", name);
          sprintf (buffer, "echo Dodgy address at %s `date` >>$TBG/log\n",
                   name);
          system (buffer);
          exit (1);
        }
      switch (target)
        {
#ifdef KIPPER
        case MAILING_LIST:
          sprintf (buffer, "%s/list.html", getenv ("SPQR"));
          tmp = fopen (buffer, "a");
          if (!tmp)
            {
              printf ("Can't open mailing list file\n");
              exit (1);
            }
          fprintf (tmp, "<pre>%s</pre><HR>\n", name);
          sprintf (buffer, "%s/bin/upload3", getenv ("TBG"));
          fclose (tmp);
          break;
        case GET_RULES:
          sprintf (buffer, "echo Rules to %s at `date` >>$TBG/log\n", name);
          system (buffer);
          sprintf (buffer,
                   "$HOME/bin/sm $TBG/.elm/elmheaders \"TBG Rules\" %s $HOME/www/Rules.html",
                   name);
          break;
        case GET_TIMES:
          sprintf (buffer,
                   "$HOME/bin/sm $TBG/.elm/elmheaders \"TBG Times\" %s $HOME/www/Times.html",
                   name);
          break;
#endif
        default:
#ifdef CORRECT
          if (target == 0)
            break;
#endif
          sprintf (buffer,
                   "echo \"(To %s from %s).\n%s\" | mail -s \"TBG: Anon Echo\" tbg-moderator@asciiking.com",
                   target < 200 ? names[target] : "account", from, name);
          system (buffer);
          tmp = fopen ("/tmp/z", "w");
          if (!tmp)
            {
              printf ("Can't create temp mail file\n");
              exit (1);
            }
          fprintf (tmp, "%s\n\n%s",
                   "(This is forwarded anonymous mail:\nYou CANNOT reply to it with your mailer's reply command)",
                   name);
          fclose (tmp);
          sprintf (buffer,
                   "mail -s \"TBG Anon Mail\" %s -- -f nobody@asciiking.com </tmp/z",
                   addresses[accounts[target]]);
        }
      system (buffer);
      break;
    case -8:
      if (check_msg_done (id))
        return;
      sprintf (buffer, "mail -s \"TBG URL\" %s <$TBG/secrets/%d/%s",
               addresses[accounts[target]], game, names[target]);
      system (buffer);
      break;
    case -3:
      if (check_msg_done (id))
        return;
      sprintf (buffer,
               "mail -s \"TBG Turn %d\" %s <$TBG/results/%d/%s%d.html",
               last_turn, addresses[accounts[target]], game, names[target],
               last_turn);
      system (buffer);
      break;
#ifdef KIPPER
    case -5:
      if (check_msg_done (id))
        return;
      sprintf (buffer,
               "cat $TBG/multi/header $TBG/results/%d/%s%d.html | HOME=$TBG/multi elm -s \"TBG Turn %d\" %s",
               game, names[target], last_turn, last_turn, addresses[target]);
      system (buffer);
      break;
#endif
    case -4:
      if (check_msg_done (id))
        return;
      sprintf (buffer, "mail -s \"TBG Orders %d\" %s <$TBG/orders/%d/%s%d",
               last_turn + 1, addresses[accounts[target]], game,
               names[target], last_turn + 1);
      system (buffer);
      break;
#ifdef KIPPER
    case -6:
      if (check_msg_done (id))
        return;
      sprintf (buffer, "echo @@@ %s >>%s/incoming", name, getenv ("SPQR"));
      system (buffer);
      sprintf (buffer, "cat /tmp/tbgtmp >>%s/incoming", getenv ("SPQR"));
      system (buffer);
      break;
#endif
    default:
      if (strstr (from, ".byu."))
        {
          sprintf (buffer, "echo %s from %s `date` >>$TBG/log\n", name, from);
          system (buffer);
        }
      if (check_msg_done (id))
        return;
      valid_orders = check_key (name, turn, key);
      /* acknowledge orders */
      target = 0;
      while (target < max_player && strcmp (names[target], name))
        target++;
      if (target == max_player)
        sprintf (buffer, "echo bad orders from %s >>$TBG/log\n", name);
      else if (valid_orders && (preferences[target] & 16))
        {
          sprintf (buffer, "mail -s \"TBG %d received\" %s <$TBG/ack.txt\n",
                   turn, addresses[accounts[target]]);
          system (buffer);
        }
      sprintf (buffer, "mv --backup=numbered /tmp/tbgtmp %s/orders/%d/%s%d",
               getenv ("TBG"), game, name, turn);
      system (buffer);
      break;
    }
  update_files (game);
}

void
split_messages ()
{
  FILE *fd, *temp;
  char buffer[65000], reply[132], from[132], subject[132], id[256];
  char *p;
  int count = 1;

  fd = fopen (mailbox, "r");
  if (!fd)
    {
      printf ("Can't open mailbox\n");
      return;
    }
  fgets (buffer, 1024, fd);
  if (strncmp (buffer, "From ", 5) != 0)
    return;
  while (parse_header (fd, buffer, from, reply, subject, id))
    {
      if (strncmp (subject, "From ", 5) == 0)
        {
          char *status;

          p = buffer;

          do
            {
              status = fgets (p, 10000, fd);
              p += strlen (p);
              if (p[-1] == '\n' && p[-2] == '=')
                p -= 2;
            }
          while (status && status[0] != '\n');
          temp = fopen ("/tmp/x", "w");
          fprintf (temp, "#=%s\n", from);
          format (temp, buffer);
          fclose (temp);
          temp = fopen ("/tmp/x", "r");
          copy_message (temp, id, from);
          fclose (temp);
          do
            fgets (buffer, 1024, fd);
          while (!feof (fd) && strncmp (buffer, "From", 4));
        }
      else
        copy_message (fd, id, from);
      fgets (buffer, 1024, fd);
    }
  fclose (fd);
}


void
refresh_pages ()
{
  int p;

  for (p = 200; p < MAX_ACCOUNT; p++)
    make_player_page (p);
}


int
main (int argc, char *argv[])
{
  mailbox = argv[1];
  split_messages ();
  refresh_pages ();
  return (0);
}
