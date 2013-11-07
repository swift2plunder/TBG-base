#include "tbg.h"
#include "bytes.h"
#include <time.h>

#ifdef HAVE_LIBGSL
#include <gsl/gsl_rng.h>  /* GNU scientific library random numbers */
#include <gsl/gsl_randist.h>
gsl_rng *random_generator;

void
init_rng()
{
  session_id = (turn & 0xfff << 20) + (time (0) & 0xfffff);
  random_generator = gsl_rng_alloc(gsl_rng_ranlxs2);
  gsl_rng_set(random_generator, turn + seed);
}

int
dice (int scale)
{
  if (scale <= 0)
    return 0;
  return gsl_rng_uniform_int (random_generator, scale);
}

uint32
rand32 ()
{
  int x = gsl_rng_uniform_int(random_generator, 1<<16);
  int y = gsl_rng_uniform_int(random_generator, 1<<16);
  return (x << 16) | y;
}

int
fuzz (int in)
{
  int result;
  double x = in + ((in < 0) ? -0.5 : 0.5);
  double r = 1.0 + gsl_ran_gaussian(random_generator, 0.05);
  result = (int) x * r;
  return result;
}

int
rand_exp (double scale)
{
  double x;
  if (scale <= 0)
    return 0;
  x = gsl_ran_exponential (random_generator, scale/2);
  return x + 0.5;
}


#else

#include <stdlib.h>

void
init_rng()
{
  srand(turn + seed);
}
#ifdef kipper
int
dice (int scale)
{
  long x = scale * (RAND_MAX/scale);
  long y;
  do
    {
      y = random();
    }
  while (y < x);
  return (y % scale);
}
#else

#define HOST_SCALE      ((RAND_MAX >> 15) + 1)
int     dice(int scale)
{
        return(((rand()/HOST_SCALE) * scale) >> 15);
}

#endif

uint32
rand32 ()
{
  int x = dice(1<<16);
  int y = dice(1<<16);
  return (x << 16) | y;
}

int
fuzz (int in)
{
  int x = in/10;
  int d = 2*x;
  return in - d + dice(1+x);
}

int
rand_exp (double scale)
{
  unsigned int r = random ();
  int i;
  int m = 1;

  for (i = 0 ; i < 32  && (r & (1 << i)); i++)
    m *= 2;
  return dice(m*scale);
}
#endif
