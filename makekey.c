#include <stdlib.h>
#include <stdio.h>
#include <rpc/des_crypt.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include "bytes.h"
#include "rand.h"

int
main ()
{
  int k;

  k = make_key ("Shadowdancer", 20);
  printf ("%d\n", k);
  exit (0);
}

