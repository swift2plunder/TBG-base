#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include "bytes.h"

#define FALSE   0
#define TRUE ~FALSE

const char *webroot = "/home/tbg/work/WWW";

int
get_entry (FILE *file, char *buffer, unsigned int size)
{
  int c;
  char *buf = buffer;
  char temp[2];
  int i;
  int partial = 0;
  while ((c = fgetc(file)) && size--)
    {
      partial = partial || (c != EOF);
      switch (c)
        {
        case '+':
          *buf++ = ' ';
          break;
        case '%':
          for (i = 0 ; i < 2 ; i++)
            {
              c = fgetc(file);
              if (c == EOF)
                {
                  fprintf(stderr,
                          "End of file in the middle of URL-decoding a %%\n");
                  exit(-1);
                }
              temp[i] = c;
            }
          *buf++ = hex_to_char(temp);
          break;
        case EOF:
          *buf = '\0';
          return partial;
        case '\n':
        case '&':
          *buf = '\0';
          /*          printf("buffer = %s\n", buffer); */
          return 1;
        default:
          *buf++ = c;
          break;
        }
    }
  return 0;
}


int
check_key (char *name, int turn, int key)
{
  int checksum;

  checksum = make_key(name,turn);
  if (checksum != key)
    {
      /* forged orders ! */
      name[0] = '_';
      /* stop it being acceptable */
      return (FALSE);
    }
  return (TRUE);
}



void
copy_message (FILE * fd)
{
  FILE *tmp;
  char buffer[65000];
  char tempfilename[100];
  int turn = -10, game = -1;
  int key = 0;
  char name[65000];
  char *p;
  int valid_orders = FALSE;
  int file_descriptor;
  
  char obuf[65000];
  int ochars = 0;

  char fbuf[65000];
  int fchars = 0;

  int retval = 0;
  int error = 0;
  char ebuf[8192];
  
  name[0] = '\0';

  retval = snprintf (fbuf + fchars, 64999 - fchars,
                     "#=%s\n", getenv ("REMOTE_ADDR"));
  if (retval > 0)
    fchars += retval;
  else
    error = 1;
  while (get_entry(fd, buffer, 65000) && ! error)
    {
      /*fprintf (stderr, "buffer = %s\n", buffer);*/
      if (strncmp (buffer, "n=", 2) == 0)
        {
          strcat (name, buffer + 2);
        }
      if (strncmp (buffer, "k=", 2) == 0)
        key = atoi (buffer + 2);
      if (strncmp (buffer, "Z=", 2) == 0)
        game = atoi (buffer + 2);
      if (strncmp (buffer, "z=", 2) == 0)
        turn = atoi (buffer + 2);

      retval = snprintf (fbuf + fchars, 64999 - fchars,
                         "%s\n", buffer);
      if (retval > 0)
        fchars += retval;
      else
        {
          error = 2;
          break;
        }

      retval = snprintf (obuf + ochars, 64999 - ochars,
                         "    <li>%s<br>\n", buffer);
      if (retval > 0)
        ochars += retval;
      else
        {
          error = 3;
          break;
        }
    }

  game = 1;

  valid_orders = check_key (name, turn, key);
  
  if (!error)
    do
      {
        char ofname[2048];
        int ofnamechars = 0;
        char newfilename[2048];
        struct tm tm_now;
        time_t time_now = time(0);
        struct tm *tm_ret;
        int ofd;
        FILE *ofile;


        tm_ret = localtime_r (&time_now, &tm_now);
        if (! tm_ret)
          {
            error = 4;
            break;
          }

        retval = snprintf (ofname, sizeof (ofname),
                           "%s/orders/%d/%s%d", webroot, game, name, turn);
        if (retval > 0)
          ofnamechars += retval;
        else
          {
            error = 5;
            strerror_r (errno, ebuf + strlen(ebuf), 8192 - strlen(ebuf));
            snprintf (ebuf + strlen(ebuf), 8191 - strlen(ebuf), "<br>\n" );
            break;
          }
        sprintf (newfilename, "%s", ofname);
        retval = strftime (newfilename + ofnamechars,
                           sizeof (newfilename) - ofnamechars,
                           "-%F-%T-XXXXXX", &tm_now);
        if (retval == 0)
          {
            error = 6;
            strerror_r (errno, ebuf + strlen(ebuf), 8192 - strlen(ebuf));
            snprintf (ebuf + strlen(ebuf), 8191 - strlen(ebuf), "<br>\n" );
            break;
          }
        ofd = mkstemp (newfilename);
        if (ofd == -1)
          {
            error = 7;
            strerror_r (errno, ebuf + strlen(ebuf), 8192 - strlen(ebuf));
            snprintf (ebuf + strlen(ebuf), 8191 - strlen(ebuf), "<br>\n" );
            break;
          }
        ofile = fdopen (ofd, "w");
        if (! ofile)
          {
            error = 8;
            strerror_r (errno, ebuf + strlen(ebuf), 8192 - strlen(ebuf));
            snprintf (ebuf + strlen(ebuf), 8191 - strlen(ebuf), "<br>\n" );
            break;
          }
        retval = fprintf (ofile, "%s", fbuf);
        if (retval < 0)
          {
            error = 9;
            strerror_r (errno, ebuf + strlen(ebuf), 8192 - strlen(ebuf));
            snprintf (ebuf + strlen(ebuf), 8191 - strlen(ebuf), "<br>\n" );
            break;
          }
        fclose (ofile);
        chmod (newfilename, 0770);
        chown (newfilename, 33, 119);
        if (unlink(ofname))
          {
            if (errno != ENOENT)
              {
                error = 10;
                strerror_r (errno, ebuf + strlen(ebuf), 8192 - strlen(ebuf));
                snprintf (ebuf + strlen(ebuf), 8191 - strlen(ebuf), "<br>\n" );
                break;
              }
          }
        if (symlink(newfilename, ofname))
          {
            error = 11;
            strerror_r (errno, ebuf + strlen(ebuf), 8192 - strlen(ebuf));
            snprintf (ebuf + strlen(ebuf), 8191 - strlen(ebuf), "<br>\n" );
            break;
          }
      }
    while (0);
  
  /* acknowledge orders */
  printf("Content-Type: text/html\n\n");
  printf("<html>\n <head>\n  <title>TBG Order Submission</title>\n");
  printf(" </head>\n <body>\n");
  /*
    printf("<p>Name: %s<br>Turn: %d<br>Key: %d</p>", name, turn,
         key); 
  */
  if (valid_orders && ! error)
    {
      printf("  <p>Order submission successful</p>\n");
    }
  else
    {
      printf("  <h1>Order submission <strong>un</strong>successful</h1>\n");
    }
  if (! valid_orders)
    printf ("  <p>Incorrect key submitted. Are you trying to cheat?</p>\n");
  if (error)
    {
      printf ("  <p>An internal error occured:  code #%d</p>\n", error);
      printf ("  <p>Error message(s):<br>%s</p>", ebuf);
    }
  if (valid_orders && ! error)
    {
      printf("  <p>Submitted orders:\n   <ul>\n%s   </ul>\n  </p>\n", obuf);
    }
  printf(" </body>\n</html>\n");
}



int
main (int argc, char *argv[])
{
  umask (0);
  copy_message (stdin);
  return (0);
}
