#ifndef BYTES_H
#define BYTES_H 1

#include "config.h"

/* From bytes.c */
#if (SIZEOF_CHAR == 1)
typedef unsigned char byte;
#else
#error No 8-bit type
#endif


#if (SIZEOF_INT == 4)
typedef unsigned int uint32;
#elif (SIZEOF_SHORT == 4)
typedef unsigned short uint32;
#elif (SIZEOF_LONG == 4)
typedef unsigned long uint32;
#else
#error No 32-bit type
#endif

#if (SIZEOF_LONG == 8)
typedef unsigned long uint64;
#elif (SIZEOF_LONG_LONG == 8)
typedef unsigned long long uint64;
#else
#error No 64-bit type
#endif

const char * byte_name (int number);
const char * uint32_name (uint32 number);
const char * uint64_name (uint64 number);
uint32 turn_code (int turn, int alien);
int public_password (int pin);
int make_key (char *name, int turn);
int bitcount (int map);
uint32 bit32 (int);
uint64 bit64 (int);

int hex (char ch);
char hex_to_char (char *buffer);

void set_bit (uint32 *array, int n);
void reset_bit (uint32 *array, int n);
int get_bit (uint32 *array, int n);
byte get_byte (uint32 *array, int n);
void set_byte (uint32 *array, int n, byte b);

#endif
