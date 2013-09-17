#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include "bytes.h"
#include "rand.h"

int
weighted_tech ()
{
  int rand_int;

  rand_int = dice (20) + 1;
  switch (rand_int)
    {
    case 1:
    case 2:
    case 3:
    case 4:
      return (1);
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
      return (2);
    case 11:
    case 12:
    case 13:
    case 14:
      return (3);
    case 15:
    case 16:
    case 17:
      return (4);
    case 18:
    case 19:
      return (5);
    case 20:
      return (6);
    default:
      return (1);
    }
}

int
main ()
{
  int i;
  for (i = 0; i < 100; i++)
    {
    printf ("%d\n", weighted_tech());
    }
  exit (0);
}

