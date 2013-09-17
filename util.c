#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

int
min (int a, int b)
{
  return (a > b ? b : a);
}

int
max (int a, int b)
{
  return (a > b ? a : b);
}

int
isqrt (int x)
{
  int result = 1;

  while (result * result <= x)
    result++;
  return (result - 1);
}


void
force_symlink (const char *oldpath, const char *newpath)
{
  if (unlink(newpath))
    {
      if (errno != ENOENT)
        {
          perror("force_symlink (unlink)");
        }
    }
  if (symlink(oldpath, newpath))
    {
      perror("force_symlink (symlink)");
    }
}

const char *
name_string (const char *name)
{
  static char buffer[256];
  const char *p;
  char *q;

  p = name;
  q = buffer;
  while (*p)
    {
      switch (*p)
        {
        case '_':
          *q++ = ' ';
          break;
        case '@':
          *q++ = '\'';
          break;
        default:
          *q++ = *p;
          break;
        }
      p++;
    }
  *q = '\0';
  return (buffer);
}

const char *
url_shipname (const char *name)
{
  static char buffer[256];
  const char *p;
  char *q;

  p = name;
  q = buffer;
  while (*p)
    {
      switch (*p)
        {
        case '_':
          *q++ = '+';
          break;
        case '@':
          *q++ = '\'';
          break;
        default:
          *q++ = *p;
          break;
        }
      p++;
    }
  *q = '\0';
  return (buffer);
}

void
standardise_name (char *name, char *buffer)
{
  if (*name >= 'a' && *name <= 'z')
    *name -= 32;                /* go to upper case initial */
  if (*name == 'S' && name[1] == '#')
    {
      sprintf (buffer, "Star #%d", atoi (name + 2));
      return;
    }
  *buffer++ = *name++;
  while (*name)
    {
      if (*name >= 'A' && *name <= 'Z')
        *name += 32;            /* go to lower case */
      if (*name == '\n')
        name++;
      else
        *buffer++ = *name++;
    }
  *buffer = '\0';
}


void
html_header (FILE * fd, char *web_source)
{
  fprintf (fd, "<BASE HREF=\"%s\"></HEAD>\n", web_source);
  fprintf (fd,
           "<BODY TEXT=\"Yellow\" BACKGROUND=\"%sstars.gif\" BGCOLOR=\"Black\" LINK=\"White\" VLINK=\"Cyan\"><CENTER>\n",
           web_source);
}

