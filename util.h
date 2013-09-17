#ifndef UTIL_H
#define UTIL_H 1
int min (int, int);
int max (int, int);
int isqrt (int);
void force_symlink (const char *oldpath, const char *newpath);
const char *name_string (const char *name);
const char *url_shipname (const char *name);
void standardise_name (char *name, char *buffer);
void html_header (FILE * fd, char *);

#endif
