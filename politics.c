#include "defs.h"
#include "tbg.h"
#include "globals.h"
#include "politics.h"
#include "util.h"
#include "aliens.h"
#include "rand.h"
#include "tbg-big.h"
#include "locations.h"
#include <stdlib.h>

int
minister_mod (struct PLAYER *player, int value, item_sort sort)
{
  int bonus = value / 2;
  int race = who_home (player->star);

  if (race == -1
      || races[race].religion != repairers[sort]
      || ministers[races[race].minister] != player - players)
    return value;
  if (player->enemies & (1 << race))
    bonus /= 2;
  return (value + bonus);
}

int
aliens_report (int star, int minister, int turn)
{
  int a;
  int race;

  for (a = 0; a < MAX_ALIEN; a++)
    {
      race = aliens[a].alliance;
      if (aliens[a].star == star
          && (turn_code(turn,a) % 30) == (turn % 30)
          && races[race].minister == minister)
        return (TRUE);
    }
  return (FALSE);
}

void
alien_reports ()
{
  int star;
  FILE *fd;
  char buffer[500];
  int minister;

  for (minister = 0 ; minister < 9 ; minister++)
    {
      int reported = 0;
      if (minister == PRESIDENT)
        continue;
      snprintf (buffer, 500,
                "%s/results/%d/minister%d-report%d.html",
                webroot, game, minister, turn);
      fd = fopen (buffer, "w");
      if (!fd)
        {
          printf ("Can't open report file\n");
          exit (1);
        }
      fprintf (fd, "<html><head><title>Alien Report, Turn %d</title>\n", turn);
      html_header (fd, players[ministers[minister]].web_source);
      fprintf (fd, "<h1>Alien Report, Turn %d</h1>\n", turn);
      for (star = 0; star < MAX_STAR; star++)
        {
          if (aliens_report (star, minister, turn)
              && star_seen (players + ministers[minister], star))
            {
              show_starsystem (fd, players, star);
              reported = 1;
            }
        }
      if (! reported)
        fprintf(fd, "<p>Sorry, no reports this turn</p>\n");
      fprintf(fd, "</body></html>\n");
      fclose (fd);
    }
}

void
generate_voting_options (FILE * fd, struct PLAYER *player)
{
  int p;

  if (turn - player->last_restart < 100)
    {
      fprintf (fd, "<div class=\"tribune\"><table><tr><th>Vote for ");
      print_rules_link (fd, "Tribune", "Tribune");
      fprintf (fd, " of the People</th></tr>\n");
      fprintf (fd, "<tr><th>(Votes last turn shown in brackets)</th></tr>\n");
      fprintf (fd, "<tr><td><select name=\"P\">\n");
      fprintf (fd, "<option value=T0>No-one\n");
      for (p = 1; p < MAX_PLAYER; p++)
        if ((turn - players[p].last_restart < 100)
            && (players[p].plebs || player - players == p)
            && p != ministers[VEEP]
            && p != ministers[PRESIDENT])
          fprintf (fd, "<option value=T%d %s>%s (%d)</option>\n",
                   p,
                   p == player->trib ? "selected" : "",
                   name_string (players[p].name),
                   players[p].plebs);
      fprintf (fd, "</select></td></tr></table></div>\n");
    }
  if (turn % 10 == 9)
    {
      fprintf (fd, "<div class=\"politics\" style=\"bottom-margin:1.5em;\"><h2>Election Time - Choose a President!</h2>\n");
      fprintf (fd, "<select name=\"P\">\n");
      fprintf (fd, "<option value=\"Z0\">No-one</option>\n");
      for (p = 1; p < MAX_PLAYER; p++)
        if (players[p].politics & CANDIDATE)
          fprintf (fd, "<option value=\"Z%d\">%s</option>\n",
                   p, name_string (players[p].name));
      fprintf (fd, "</select></div>\n");
    }
}

void
generate_presidential_options (FILE * fd, struct PLAYER *player)
{
  skill_sort skill;
  int p;

  if (ministers[PRESIDENT] == player - players)
    {
      fprintf (fd, "<div class=\"politics\"><table><tr><th>As President</th></tr>\n");
      for (skill = engineering; skill <= weaponry; skill++)
        {
          fprintf (fd, "<th>Appoint %s Minister</th>\n", skill_names[skill]);
          fprintf (fd, "<tr><td><select name=\"P\">\n");
          for (p = 1; p < MAX_PLAYER; p++)
            if ((players[p].politics & CANDIDATE)
                && !(players[p].politics & CENSORED)
                && ministers[VEEP] != p
                && ministers[TRIBUNE] != p)
              fprintf (fd, "<option value=\"Q%d\" %s>%s</option>\n",
                       MAX_PLAYER * skill + p,
                       ministers[skill] == p ? "selected" : "",
                       name_string (players[p].name));
          fprintf (fd, "</select></td></tr>\n");
        }

      fprintf (fd, "<th>Appoint Industry Minister</th>\n");
      fprintf (fd, "<tr align=center><td><select name=\"P\">\n");
      for (p = 1; p < MAX_PLAYER; p++)
        if ((players[p].politics & CANDIDATE)
            && !(players[p].politics & CENSORED)
            && ministers[VEEP] != p
            && ministers[TRIBUNE] != p)
          fprintf (fd, "<option value=\"Q%d\" %s>%s</option>\n",
                   MAX_PLAYER * MIN_IND + p,
                   ministers[MIN_IND] == p ? "selected" : "",
                   name_string (players[p].name));
      fprintf (fd, "</select></td></tr>\n");

      fprintf (fd, "<th>Appoint Justice Minister</th>\n");
      fprintf (fd, "<tr><td><select name=\"P\">\n");
      for (p = 1; p < MAX_PLAYER; p++)
        if ((players[p].politics & CANDIDATE)
            && !(players[p].politics & CENSORED)
            && ministers[VEEP] != p
            && ministers[TRIBUNE] != p)
          fprintf (fd, "<option value=\"Q%d\" %s>%s</option>\n",
                   MAX_PLAYER * MIN_JUST + p,
                   ministers[MIN_JUST] == p ? "selected" : "",
                   name_string (players[p].name));
      fprintf (fd, "</select></td></tr>\n");

#if 0
      fprintf (fd, "<th>Propose Shop Tech Level</th>\n");
      fprintf (fd, "<tr align=center><td><SELECT NAME=\"P\">\n");
      for (p = 1; p < 7; p++)
        fprintf (fd, "<OPTION VALUE=\"R%d\" %s>%s\n",
                 100 * PRESIDENT + p,
                 p == restock_tech ? "selected" : "", tech_level_names[p]);
      fprintf (fd, "</SELECT></td></tr>\n");

      fprintf (fd, "<th>Propose Shop Modules</th>\n");
      fprintf (fd, "<tr align=center><td><SELECT NAME=\"P\">\n");
      for (p = warp_drive; p < artifact; p++)
        fprintf (fd, "<OPTION VALUE=\"S%d\" %s>%s\n",
                 100 * PRESIDENT + p,
                 p == proposed_types[PRESIDENT] ? "selected" : "",
                 item_names[p]);
      fprintf (fd, "</SELECT></td></tr>\n");
#endif

      fprintf (fd, "</table></div>\n");
    }
  if (ministers[VEEP] == player - players)
    {
      fprintf (fd, "<div class=\"politics\"><table><tr><th>As Vice President</th></tr>\n");

      fprintf (fd, "<th>Propose Shop Tech Level</th>\n");
      fprintf (fd, "<tr align=center><td><select name=\"P\">\n");
      for (p = 1; p < 7; p++)
        fprintf (fd, "<option value=\"R%d\" %s>%s</option>\n",
                 100 * VEEP + p,
                 p == proposed_techs[VEEP] ? "selected" : "",
                 tech_level_names[p]);
      fprintf (fd, "</select></td></tr>\n");

      fprintf (fd, "<th>Propose Shop Modules</th>\n");
      fprintf (fd, "<tr align=center><td><select name=\"P\">\n");
      for (p = warp_drive; p < artifact; p++)
        fprintf (fd, "<option value=\"S%d\" %s>%s</option>\n",
                 100 * VEEP + p,
                 p == proposed_types[VEEP] ? "selected" : "",
                 item_names[p]);

      fprintf (fd, "</select></td></tr>\n");

      fprintf (fd, "</table></div>\n");
    }

  if (ministers[TRIBUNE] == player - players)
    {
      fprintf (fd, "<div class=\"tribune\"><table><tr><th>As Tribune</th></tr>\n");

      fprintf (fd, "<th>Propose Shop Tech Level</th>\n");
      fprintf (fd, "<tr align=center><td><select name=\"P\">\n");
      for (p = 1; p < 3; p++)
        fprintf (fd, "<option value=\"R%d\" %s>%s</option>\n",
                 100 * TRIBUNE + p,
                 p == proposed_techs[TRIBUNE] ? "selected" : "",
                 tech_level_names[p]);
      fprintf (fd, "</select></td></tr>\n");

      fprintf (fd, "<th>Propose Shop Modules</th>\n");
      fprintf (fd, "<tr align=center><td><select name=\"P\">\n");
      for (p = warp_drive; p < artifact; p++)
        fprintf (fd, "<option value=\"S%d\" %s>%s</option>\n",
                 100 * TRIBUNE + p,
                 p == proposed_types[TRIBUNE] ? "selected" : "",
                 item_names[p]);
      fprintf (fd, "</select></td></tr>\n");

      fprintf (fd, "</table></div>\n");
    }

  for (skill = engineering; skill <= weaponry; skill++)
    {
      if (ministers[skill] == player - players)
        {
          int i = restock_item;
          fprintf (fd,
                   "<div class=\"politics\"><table><tr><th>As %s Minister</th></tr>\n",
                   skill_names[skill]);
          fprintf (fd, "<th>Propose Shop Tech Level</th>\n");
          fprintf (fd, "<tr align=center><td><select name=\"P\">\n");
          for (p = 1; p < 7; p++)
            fprintf (fd, "<option value=\"R%d\" %s>%s</option>\n",
                     100 * skill + p,
                     p == restock_tech ? "selected" : "",
                     tech_level_names[p]);
          fprintf (fd, "</select></td></tr>\n");
          fprintf (fd, "<th>Propose Shop Modules</th>\n");
          fprintf (fd, "<tr><td><select name=\"P\">\n");
          while (repairers[i] != skill)
            i = 1 + dice(pod);
          for (p = warp_drive; p < artifact; p++)
            if (repairers[p] == skill)
              {
                fprintf (fd, "<option value=\"S%d\" %s>%s</option>\n",
                         100 * skill + p,
                         p == proposed_types[skill] ? "selected" : "",
                         item_names[p]);
              }
          fprintf (fd, "</select></td></tr>\n");
          fprintf (fd, "</table></div>\n");
        }
    }

  if (ministers[MIN_JUST] == player - players)
    {
      fprintf (fd,
               "<div class=\"politics\"><table><tr><th>As Minister of Justice</th></tr>\n");
      fprintf (fd, "<tr><th>Judge in Favour of</th></tr>\n");
      fprintf (fd, "<tr><td><select name=\"P\">\n");
      fprintf (fd, "<option value=\"J%d\">%s (Accuser)</option>\n",
               accuser, races[accuser].name);
      fprintf (fd, "<option value=\"J%d\" selected>%s (Defendent)</option>\n",
               defendent, races[defendent].name);
      fprintf (fd, "</SELECT></td></tr>\n");
      fprintf (fd, "<tr><th>Choose Rogue Race</th></tr>\n");
      fprintf (fd, "<tr><td><select name=\"P\">\n");
      for (p = 0; p < MAX_RACE; p++)
        fprintf (fd, "<option value=\"G%d\" %s>%s</option>\n",
                 p, p == restock_rogues_race ? "selected" : "", races[p].name);
      fprintf (fd, "</select></td></tr>\n");
      fprintf (fd, "<tr><th>Choose Rogue Skill</th></tr>\n");
      fprintf (fd, "<tr align=center><td><select name=\"P\">\n");
      for (p = 0; p < 4; p++)
        fprintf (fd, "<option value=\"H%d\" %s>%s</option>\n",
                 p, p == restock_rogues_skill ? "selected" : "", skill_names[p]);
      fprintf (fd, "</select></td></tr>\n");
      fprintf (fd, "</table></div>\n");
    }
  if (ministers[MIN_IND] == player - players)
    {
      fprintf (fd,
               "<div class=\"politics\"><table><tr><th>As Minister of Industry</th></tr>\n");
      fprintf (fd, "<th>Choose shop production:</th>\n");
      fprintf (fd, "<tr><td><select name=\"P\">\n");
      for (p = 0; p < 7; p++)
        {
          if (p == PRESIDENT)
            continue;
          fprintf (fd, "<option value=\"U%d\" %s>%s %s</option>\n",
                   100 * proposed_types[p] + proposed_techs[p],
                   p == VEEP ? "selected" : "",
                   tech_level_names[proposed_techs[p]],
                   item_names[proposed_types[p]]);
        }
      fprintf (fd, "</select></td></tr>\n");
      fprintf (fd, "</table></div>\n");
    }
}

void
political_command (struct PLAYER *player, char command, int param)
{
  int i, p, m;
  switch (command)
    {
    case 'Z':                   /* presidential vote */
      total_votes[param] += player->votes;
      break;
    case 'T':                   /* tribune vote */
      players[param].plebs++;
      player->trib = param;
      break;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
      ministers[command - 'A'] = param;
      break;
    case 'E':
      restock_tech = param;
      break;
    case 'F':
      restock_item = param;
      break;
    case 'G':
      restock_rogues_race = param;
      break;
    case 'H':
      restock_rogues_skill = param;
      break;
    case 'J':
      case_winner = param;
      break;
    case 'O':
      players[param].politics |= CENSORED;
      break;
    case 'Q':
      p = param % MAX_PLAYER;
      m = param / MAX_PLAYER;
      for (i = 0 ; i < 9 ; i++)
        {
          if (i != PRESIDENT && ministers[i] == p)
            ministers[i] = 0;
        }
      ministers[m] = p;
      break;
    case 'R':
      m = param / 100;
      p = param % 100;
      proposed_techs[m] = p;
      break;
    case 'S':
      m = param / 100;
      p = param % 100;
      proposed_types[m] = p;
      break;
    case 'U':
      restock_item = param / 100;
      restock_tech = param % 100;
      break;
    default:
      printf ("%s makes political command %c, parameter %d\n",
              player->name, command, param);
    }
}

void
make_president_link (FILE * fd, int minister)
{
  int unique_url;
  char buf1[256];
  char buf2[256];

  unique_url = (rand32() & ~7) | minister;

  snprintf (buf1, 256, "%s/results/%d/minister%d-report%d.html",
            webroot, game, minister, turn);
  snprintf (buf2, 256, "%s/Report_%s%d.htm",
            webroot, uint32_name (unique_url), turn);
  force_symlink(buf1, buf2);
  
  fprintf (fd,
           "<a href=\"http://%s/Report_%s%d.htm\">Report from Alien ships</a>",
           server, uint32_name (unique_url), turn);
}

void
check_votes (FILE * fd, struct PLAYER *player)
{
  int loc;
  int votes = 0;
  int i;

  fprintf (fd, "<div class=\"politics\"><h3>");

  print_rules_link (fd, "The_Galactic_Council", "Politics");
  fprintf (fd, "</h3>\n");

  for (i = 0 ; i < 9 ; i++)
    {
      if (i != PRESIDENT && ministers[i] == player - players)
        make_president_link (fd, i);
    }

  if (player->votes == 0)
    {
      fprintf (fd, "<p>You control no Presidential votes</p></div>\n");
    }
  else
    {
      fprintf (fd, "<table>\n");
      fprintf (fd,
               "<tr><th colspan=\"2\">You control %d Presidential votes</th></tr>\n",
               player->votes);
      fprintf (fd, "<tr><th>Location</th><th>Influence</th></tr>\n");
      for (loc = 0; loc < MAX_LOCATION; loc++)
        {
          if (locations[loc].voter == player - players)
            {
              int race;
              if (locations[loc].sort == colony)
                race = RACE_NUMBER (locations[loc].parameter);
              else
                race = locations[loc].parameter;
              votes += locations[loc].votes;
              fprintf (fd,
                       "<tr><td>%s %s (%d) at %s</td><td>%d</td></tr>\n",
                       races[race].name, loc_string (loc), loc,
                       star_names[locations[loc].star],
                       locations[loc].influence);
            }
        }
      fprintf (fd, "</table></div>\n");
    }
}

void
consolidate_votes ()
{
  int p, loc;

  for (p = 0; p < MAX_PLAYER; p++)
    players[p].votes = 0;
  for (loc = 0; loc < MAX_LOCATION; loc++)
    players[locations[loc].voter].votes += locations[loc].votes;
  for (p = 0; p < MAX_PLAYER; p++)
    if (players[p].votes >= 10)
      players[p].politics |= CANDIDATE;
}

void
do_election ()
{
  int i, winner = 0, runner_up = 0;

  if (turn % 10 == 0)
    {
      consolidate_votes ();
      for (i = 1; i < MAX_PLAYER; i++)
        if (winner == 0 || total_votes[i] > total_votes[winner])
          {
            runner_up = winner;
            winner = i;
          }
        else if (runner_up == 0 || total_votes[i] > total_votes[runner_up])
          {
            runner_up = i;
          }
      if (winner == 0)
        winner = ministers[PRESIDENT];
      if (runner_up == 0)
        runner_up = ministers[VEEP];

      fprintf (times, "<hr><div class=\"politics\"><h2>The new President is %s!</h2>",
               name_string (players[winner].name));
      ministers[PRESIDENT] = winner;
      players[winner].politics |= PROCONSUL;

      fprintf (times, "<h3>The new Vice President is %s!</h3>",
               name_string (players[runner_up].name));
      ministers[VEEP] = runner_up;

      fprintf (times, "<h4>Votes for each candidate were:</h4>\n");
      fprintf (times, "<ul><li>No-one: %d</li>\n", total_votes[0]);
      for (i = 1; i < MAX_PLAYER; i++)
        {
          if (total_votes[i])
            fprintf (times, "<li>%s: %d</li>\n",
                     name_string (players[i].name), total_votes[i]);
          players[i].politics &= KEEP_POLITICS_FLAGS;
        }
      fprintf (times, "</ul></div>\n");
    }
  consolidate_votes ();
}

void
do_tribunal_election ()
{
  int highest = -1;
  int winner = 0;
  int p;

  for (p = 1; p < MAX_PLAYER; p++)
    {
      if ((players[p].plebs > highest)
          || (players[p].plebs == highest
              && players[p].last_restart > players[winner].last_restart))
        {
          winner = p;
          highest = players[winner].plebs;
        }
    }
  ministers[TRIBUNE] = winner;
  for (p = 1 ; p < 9 ; p++)
    {
      if (ministers[p] == winner
          && p != TRIBUNE)
        ministers[p] = 0;
    }
  fprintf (times,
           "<hr><h2 class=\"tribune\">The new Tribune of the People is %s!</h2>\n",
           name_string (players[winner].name));
}


void
resolve_judgement ()
{
  int loc, case_loser;

  fprintf (times,
           "<hr><div class=\"politics\">Hear ye! Hear ye! Hear ye!<br>The case of %s vs %s is judged in favour of the %s nation<br>(The honourable Judge %s presiding)\n",
           races[accuser].name, races[defendent].name,
           races[case_winner].name,
           name_string (players[ministers[MIN_JUST]].name));

  case_loser = accuser == case_winner ? defendent : accuser;

  for (loc = 0; loc < MAX_LOCATION; loc++)
    {
      int parameter = locations[loc].parameter;
      int race = -1;
      switch (locations[loc].sort)
        {
        case colony:
          race = RACE_NUMBER (parameter);
          break;
        case homeworld:
          race = parameter;
          break;
        }
      if (locations[loc].voter == ministers[VEEP])
        {
          if (race == case_loser)
            locations[loc].influence += locations[loc].influence / 2;
          if (race == case_winner)
            locations[loc].influence /= 2;
        }
      else
        {
          if (race == case_winner)
            locations[loc].influence += locations[loc].influence / 2;
          if (race == case_loser)
            locations[loc].influence /= 2;
        }
      if (locations[loc].influence == 0)
        locations[loc].voter = 0;
    }
  accuser = dice (MAX_RACE);
  do
    defendent = dice (MAX_RACE);
  while (accuser == defendent);
  fprintf (times, "<br>The next case will be %s vs %s</div>\n",
           races[accuser].name, races[defendent].name);
}


void
influence_decay ()
{
  int loc;

  for (loc = 0; loc < MAX_LOCATION; loc++)
    {
      int race = -1;
      int votes;

      switch (locations[loc].sort)
        {
        case colony:
          race = RACE_NUMBER (locations[loc].parameter);
          break;
        case homeworld:
          race = locations[loc].parameter;
          break;
        }
      if (race == -1)
        continue;
      votes = locations[loc].influence;
      if (players[locations[loc].voter].enemies & (1 << race))
        votes = (votes * 9) / 10;
      else
        votes = (votes * 19) / 20;
      locations[loc].influence = votes;
      if (!votes)
        locations[loc].voter = 0;
    }
}

