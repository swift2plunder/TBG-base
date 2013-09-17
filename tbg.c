/* 
   tbg - Runs the web based game To Boldly Go.

   Copyright (C) 1996-2009 Eric E Moore, Jeremy Maiden

#include <termios.h>
#include <grp.h>
#include <pwd.h>
*/

#include <stdio.h>
#include <sys/types.h>
#include <argp.h>
#include "system.h"
#include "opts.h"
#include "tbg-big.h"

int
main (int argc, char **argv)
{
  textdomain(PACKAGE);
  argp_parse(&argp, argc, argv, 0, NULL, NULL);
  jm_main();

  exit (0);
}
