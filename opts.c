/* 
   tbg - Runs the web based game To Boldly Go.

   Copyright (C) 1996-2007 Eric E Moore, Jeremy Maiden

#include <termios.h>
#include <grp.h>
#include <pwd.h>
*/

#include <stdio.h>
#include <sys/types.h>
#include "system.h"
#include "opts.h"

#define EXIT_FAILURE 1

char *xmalloc ();
char *xrealloc ();
char *xstrdup ();

/* Parse a single option.  */
error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  static char * default_server = "tbg.asciiking.com";
  char *temp;
  switch (key)
    {
    case ARGP_KEY_INIT:
      /* Set up default values.  */
      desired_directory = NULL;
      webroot = NULL;
      gameroot = NULL;
      want_quiet = 0;
      want_verbose = 0;
      really_send = 1;
      send_email = 1;
      turn = -1;
      server = default_server;
      break;

    case 'q':			/* --quiet, --silent */
      want_quiet = 1;
      break;
    case 't':
      while (isspace(*arg))
        arg++;
      turn = strtol(arg,&temp,0);
      if (temp == arg)
        {
          fprintf (stderr, "Error: %s not a valid turn number\n", arg);
          exit(-1);
        }
      break;
    case 'v':			/* --verbose */
      want_verbose = 1;
      break;
    case DRYRUN_KEY:		/* --dry-run */
      really_send = 0;
      break;
    case DIRECTORY_KEY:		/* --directory */
      desired_directory = xstrdup (arg);
      break;
    case 'w':		/* --web */
      webroot = xstrdup (arg);
      break;
    case 'g':		/* --gameroot */
      gameroot = xstrdup (arg);
      break;
    case 's':		/* --server */
      server = xstrdup (arg);
      break;
    case 'e':
      send_email = 0;
      break;
    case ARGP_KEY_ARG:		/* [FILE]... */
      /* TODO: Do something with ARG, or remove this case and make
         main give argp_parse a non-NULL fifth argument.  */
      break;

    default:
      return ARGP_ERR_UNKNOWN;
    }
  return 0;
}

const char *argp_program_bug_address = "<tbg-moderator@asciiking.com>";
void (*argp_program_version_hook) (FILE *, struct argp_state *) = show_version;

/* Show the version number and copyright information.  */
void
show_version (FILE *stream, struct argp_state *state)
{
  (void) state;
  /* Print in small parts whose localizations can hopefully be copied
     from other programs.  */
  fputs(PACKAGE" "VERSION"\n", stream);
  fprintf(stream, _("Written by %s.\n\n"), "Jeremy Maiden, Eric E Moore");
  fprintf(stream, _("Copyright (C) %s %s\n"), "1996-2007",
          "Jeremy Maiden, Eric E Moore");
  fputs(_("\
This program has absolutely no warranty.\n"),
	stream);
}
