#include <cgic.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "globals.h"
#include "tbg.h"
#include "players.h"

#define OLD_MAX_ACCOUNT 600
#define MAX_ACCOUNT     600

char *
munge_name (char *name)
{
  while (strchr (name, ' '))
    *strchr (name, ' ') = '_';
  while (strchr (name, '\''))
    *strchr (name, '\'') = '@';
  while (name[strlen (name) - 1] == '_')
    name[strlen (name) - 1] = '\0';
    
  return (name);
}

int
check_name (char *name)
{
  int ok;

  if (strlen (name) < 3)
    return (FALSE);
  if (isdigit(name[strlen(name) -1]))
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
        case '\'':
        case ' ':
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
check_email (char *name)
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

void
new_player (int game, int player, int turn, char *address, char *name)
{
  FILE *fd;
  char buffer[256];
  int p;
  int max_age = 0;

  if (strchr (name, ' ') || !name[0] || !address[0])
    {
      fprintf (cgiOut,
               "Tried to add ship with a bad name or address (%s) (%s)!\n",
              name, address);
      sprintf (buffer,
               "mail -s \"TBG: Bad Shipname %s\" tbg-moderator@asciiking.com <%s/spacename",
               name, gameroot);
      system (buffer);
      exit(1);
    }
  for (p = 0; p < MAX_PLAYER; p++)
    if (strcmp (name, players[p].name) == 0)
      {
        fprintf (cgiOut,
                 "Tried to add duplicate ship %s!\n", name);
        sprintf (buffer,
                 "mail -s \"TBG: Bad Shipname %s\" tbg-moderator@asciiking.com <%s/duplicate",
                 name, gameroot);
        system (buffer);
        exit(1);
      }
  fprintf (stderr, "Checking mothballed players - player=%d\n", player);
  if (player == 0)              /* auto select */
    {
      player = 1;
      while (player < MAX_PLAYER && players[player].account_number)
        player++;
      if (player == MAX_PLAYER)
        {
          int p = 1;
          while (p < MAX_PLAYER)
            {
              fprintf (stderr, "Check player %d mothballed\n", p);
              if (mothballed(p))
                {
                  int age = turn + players[p].last_restart - 2*players[p].last_orders;
                  if (age > max_age)
                    {
                      max_age = age;
                      player = p;
                    }
                }
            p++;      /*need an incrementer in this while loop - cb*/
            }
        }
      fprintf (stderr, "Checking for room\n");
      if (player == MAX_PLAYER)
        {
          fprintf (cgiOut, "Can't add %s (%s) - no room\n", name, address);
          exit(1);
        }
    }
  fprintf (stderr, "Done checking mothballed players\n", player);
  players[player].account_number = player;
  strncpy (players[player].name, name, sizeof(players[player].name) - 1);
  strncpy (players[player].address, address, sizeof(players[player].name) - 1);

  sprintf (buffer, "%s/orders/%d/%s%d", 
           webroot, game, name, turn);
  fd = fopen (buffer, "w");
  if (!fd)
    {
      fprintf (cgiOut, "Can't create startup orders file\n");
      exit(1);
    }
  fprintf (fd, "Z=%d\n", game);
  fprintf (fd, "z=%d\n", turn);
  fprintf (fd, "n=%s\n", name);
  fprintf (fd, "y=1\n");
  fclose (fd);
}




struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory) {
    memcpy(&(mem->memory[mem->size]), ptr, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
  }
  return realsize;
}

int
cgiMain ()
{
  char name[81];
  char address[129];
  int read_rules = 0;
  int sensible_name = 0;
  
  //char recaptcha_challenge_field[8192];
  //char recaptcha_response_field[8192];

  //char *privatekey="6LfmjOYSAAAAACrkaUEgLO9yM-W4hmwyyaJaC-5u";

  int valid = 1;
  
  struct MemoryStruct chunk;
  cgiFormResultType cgi_res;

  textdomain(PACKAGE);
  gameroot="/home/tbg/work";
  desired_directory="/home/tbg/work/tbg";
  webroot="/home/tbg/work/WWW";
  turn = -1;
  
  chunk.memory=NULL; /* we expect realloc(NULL, size) to work */
  chunk.size = 0;    /* no data at this point */
  

  curl_global_init(CURL_GLOBAL_ALL);
  
  cgiHeaderContentType("text/plain");

  fflush(stderr);

  cgi_res = cgiFormStringNoNewlines("ship_name", name, 81);

  switch(cgi_res)
    {
    case cgiFormSuccess:
      break;
    case cgiFormNotFound:
      fprintf(cgiOut, "ship_name field not found\n");
      exit(1);
      break;
    case cgiFormEmpty:
      fprintf(cgiOut, "You need to provide a ship name!\n");
      exit(1);
      break;
    case cgiFormTruncated:
      fprintf(cgiOut, "Ship name too long! (max 80 characters)\n", name);
      exit(1);
      break;
    }

  if (! check_name(name))
    {
      fprintf(cgiOut, "Invalid name.  Must not end in a digit.  Valid characters are numbers, letters,\nspace, - _ and '.\n");
      fprintf(stderr, "Ship name is %s\n", name);
      valid = 0;
    }
  munge_name(name);
  
  cgi_res = cgiFormStringNoNewlines("email", address, 81);
  switch(cgi_res)
    {
    case cgiFormSuccess:
      break;
    case cgiFormNotFound:
      fprintf(cgiOut, "email field not found\n");
      exit(1);
      break;
    case cgiFormEmpty:
      fprintf(cgiOut, "You need to provide an email address!\n");
      exit(1);
      break;
    case cgiFormTruncated:
      fprintf(cgiOut, "Email address too long! (max 128 characters)\n", address);
      exit(1);
      break;
    }

  if (! check_email(address))
    {
      fprintf(cgiOut, "Email address looks bad: %s\n", address);
      valid = 0;
    }
  
  cgi_res = cgiFormInteger("read_rules", &read_rules, 0);
  cgi_res = cgiFormInteger("sensible_name", &sensible_name, 0);


  if (! read_rules)
    {
      fprintf(cgiOut, "You should read the rules before signing up.\n");
      valid = 0;
    }
  if (! sensible_name)
    {
      fprintf(cgiOut, "Please pick a sensible name\n");
      valid = 0;
    }
/*
  cgiFormStringNoNewlines("recaptcha_response_field",
                          recaptcha_response_field, 81191);

  cgiFormStringNoNewlines("recaptcha_challenge_field",
                          recaptcha_challenge_field, 8191);
  if (valid)
    {
      CURL *handle = curl_easy_init();
      char buf[4096];
      char *encoded;
      char obuf[4096];
      
      buf[4095] = 0;
      obuf[4095] = 0;
      
      snprintf(buf, 4095, "privatekey=%s&remoteip=%s&challenge=%s&response=%s\n",
               privatekey, cgiRemoteAddr,
               recaptcha_challenge_field, recaptcha_response_field);

      encoded = curl_easy_escape(handle, buf, 0);
      curl_easy_setopt(handle, CURLOPT_URL,
                       "http://api-verify.recaptcha.net/verify");
      curl_easy_setopt(handle, CURLOPT_POST, 1);
      curl_easy_setopt(handle, CURLOPT_POSTFIELDS, (void *) buf);
      curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void *)&chunk);
      curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
      
      curl_easy_perform(handle);
      
      if(chunk.size > 4 && !strncmp("true", chunk.memory, 4))
        {
        }
      else
        {
          fprintf(cgiOut, "The capcha says you are not a human being.\n");
          fprintf(cgiOut, "But it's a little finicky, so feel free to hit back and try again.\nThere's a reload button you can hit to get a nicer pair of words.\n");
          valid = 0;
        }
      
      if (chunk.memory)
        free(chunk.memory);

      curl_easy_cleanup(handle);
    }            //Commented out Recaptcha                  */
  
  if (valid)
    {
//      int ret;  // Removed an error test that seemed to be malfunctioning - cb
      
      read_master_file ();

      read_data();

      players = malloc (MAX_PLAYER * sizeof (struct PLAYER));

      printf("Attempting to add %s (%s)\n", address, name_string(name));
      read_players();
      new_player (1, 0, turn, address, name);
//      if (! ret)
        fprintf (cgiOut, "Added %s\n", name);
//      else
//        fprintf (cgiOut, "An error occured\n");
      write_players ();
      exit (0);
    }
  else
    {
      fprintf(cgiOut, "Your ship has NOT been added.\n");
      exit (1);
    }
  
}

  
