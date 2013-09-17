#ifndef OPTS_H
#define OPTS_H 1
#include "config.h"
#include <argp.h>

#if ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
#else
# define textdomain(Domain)
# define _(Text) Text
#endif
#define N_(Text) Text

/* argp option keys */
enum {DUMMY_KEY=129
      ,DRYRUN_KEY
      ,DIRECTORY_KEY
};

error_t parse_opt (int key, char *arg, struct argp_state *state);
void show_version (FILE *stream, struct argp_state *state);


static struct argp_option options[] =
  {
    { "quiet",       'q',           NULL,            0,
      N_("Inhibit usual output"), 0 },
    { "silent",      0,             NULL,            OPTION_ALIAS,
      NULL, 0 },
    { "verbose",     'v',           NULL,            0,
      N_("Print more information"), 0 },
    { "dry-run",     DRYRUN_KEY,    NULL,            0,
      N_("Take no real actions"), 0 },
    { "no-email",    'e',           NULL,            0,
      N_("Take no real actions"), 0 },
    { "directory",   DIRECTORY_KEY, N_("DIR"),       0,
      N_("Use directory DIR"), 0 },
    { "turn",        't',           N_("TURN"),      0,
      N_("Run turn number TURN"), 0},
    { "gameroot",     'g',           N_("DIR"),      0,
      N_("Use game root directory DIR"), 0},
    { "webroot",      'w',           N_("DIR"),      0,
      N_("Use web root directory DIR"), 0},
    { "server",       's',           N_("NAME"),     0,
      N_("Set server name to NAME"), 0},
    { NULL, 0, NULL, 0, NULL, 0 }
  };
/* The argp functions examine these global variables.  */
extern const char *argp_program_bug_address;
extern void (*argp_program_version_hook) (FILE *, struct argp_state *);

static struct argp argp =
  {
    options, parse_opt, N_("[FILE...]"),
    N_("Runs the web based game To Boldly Go."),
    NULL, NULL, NULL
  };

extern int want_quiet;
extern int want_verbose;
extern int really_send;
extern int send_email;
extern int turn;
extern int seed;
extern unsigned int session_id;
extern char *desired_directory;
extern char *webroot;
extern char *gameroot;
extern char *server;

#endif
