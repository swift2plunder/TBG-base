#include <stdlib.h>
#include <stdio.h>
#include <rpc/des_crypt.h>
#include <ctype.h>

void passwd2des (char *passwd, char *key); /* missing in des_crypt.h */
#include "bytes.h"

char *vowels[8] = { "a", "e", "i", "o", "u", "y", "oo", "ee" };
char *consonants[32] = {
  "b", "bl", "c", "ch", "d", "dr", "f", "fl", "g", "gr", "h", "j",
  "k", "kl", "l", "m", "n", "p", "pr", "qu",
  "r", "s", "sh", "t", "th", "tr", "v", "w", "x", "y", "z", "zh",
};

int
hex (char ch)
{
  if (ch >= 'a')
    return (ch - 'a' + 10);
  if (ch >= 'A')
    return (ch - 'A' + 10);
  return (ch - '0');
}

char
hex_to_char (char *buffer)
{
  return (16 * hex (*buffer) + hex (buffer[1]));
}


const char *
byte_name (int number)
{
  static char buffer[5];

  snprintf (buffer, 5, "%s%s",
           consonants[(number >> 3) & 31], vowels[number & 7]);
  return (buffer);
}

const char *
uint32_name (uint32 number)
{
  static char buffer[35];

  int i, n = 0;
  for (i = 3; i >= 0 ; i--)
    {
      n += snprintf(buffer + n, 32 - n,
                    "%s", byte_name ((number >> (8 * i)) & 0xff));

    }
  buffer[0] = toupper(buffer[0]);              /* change to upper case */
  return (buffer);
}

const char *
uint64_name (uint64 number)
{
  static char buffer[65];

  int i, n = 0;
  for (i = 7; i >= 0 ; i--)
    {
      n += snprintf(buffer + n, 65 - n,
                    "%s", byte_name ((number >> (8 * i)) & 0xff));

    }
  buffer[0] = toupper(buffer[0]);              /* change to upper case */
  return (buffer);
}

uint32
turn_code (int turn, int alien)
{
  char password[9];
  char key[8];
  uint32 data[2];

  snprintf(password, 9, "M`3K,H%02x", ((turn - 1) / 30) & 0xff);

  passwd2des(password, key);

  data[0] = data[1] = alien;

  ecb_crypt(key, (char *) data, 8, DES_HW|DES_ENCRYPT);
  return data[0] ^ data[1];
}

int
public_password (int pin)
{
  char password[9];
  char key[8];
  uint32 data[2];

  snprintf(password, 9, "O,[u}4rR");

  passwd2des(password, key);

  data[0] = data[1] = pin ^ 0x3e2a;

  ecb_crypt(key, (char *) data, 8, DES_HW|DES_ENCRYPT);
  return data[0] ^ data[1];
}

int
make_key (char *name, int turn)
{
  char password[9];
  char key[8];
  uint32 data[2];
  snprintf(password, 9, ";2OI%04x", turn & 0xffff);

  passwd2des(password, key);

  data[0] = data[1] = turn;
  while (*name)
    {
      data[0] = data[0] * 17 + *name++;
      data[1] = data[1] * 17 + data[0];
    }
  ecb_crypt(key, (char *) data, 8, DES_HW|DES_ENCRYPT);
  return data[0] ^ data[1];
}

/* counts the set bits in a bitmap */
int
bitcount (int map)
{
  int result = 0, bit;

  for (bit = 0; bit < 32; bit++)
    if (map & (1 << bit))
      result++;
  return (result);
}

int
bitmap_count (uint32 *array, int n_max)
{
  int i;
  int sum = 0;
  for (i = 0 ; i < n_max/32 ; i++)
    {
      sum += bitcount(array[i]);
    }
  return sum;
}


void
set_bit (uint32 *array, int n)
{
  array[n / 32] |= (1 << (n & 31));
}

void
reset_bit (uint32 *array, int n)
{
  array[n / 32] &= ~(1 << (n & 31));
}

int
get_bit (uint32 *array, int n)
{
  return array[n / 32] & (1 << (n & 31));
}

uint32
bit32 (int i)
{
  if (i < 0 || i > 31)
    {
      fprintf(stderr,"Bad argument to bit()");
      exit(-1);
    }
  return 1 << i;
}

uint64
bit64 (int i)
{
  if (i < 0 || i > 63)
    {
      fprintf(stderr,"Bad argument to bit()");
      exit(-1);
    }
  return 1 << i;
}


byte
get_byte (uint32 *array, int n)
{
  return array[n / 4] & (0xff << (n & 3));
}

void
set_byte (uint32 *array, int n, byte b)
{
  array[n / 4] &= ~(0xff << (n & 3));
  array[n / 4] |= b << (n & 3);
}
