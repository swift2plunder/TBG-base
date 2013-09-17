#ifndef TBG_BIG_H
#define TBG_BIG_H 1

#include "globals.h"

static char * mail_server = "asciiking.com";

int mothballed (int p);
void print_rules_link (FILE *fd, const char *link, const char *text);
int module_type (int sort);
const char * ring_string (int ring);
void show_good_artifact (FILE * fd, struct ITEM *item);
void show_bad_artifact (FILE * fd, struct ITEM *item);
void show_item (FILE * fd, short it);
int total_collection (struct ITEM *ship);
int blessing_mod (struct PLAYER *player, int result, item_sort sort);
void show_ship (FILE * fd, struct PLAYER *ship);
int new_criminal (int loc);
void init_ring (int ring);
void init_rings ();
void supernova (int star);
void relocate (int loc);
void generate_board ();
int local_strength (unit_sort sort, location_sort terrain, int troops);
void show_military (FILE * fd, struct PLAYER *player);
void show_factors (FILE * fd, struct PLAYER *player);
const char * pair_string (struct PLAYER *player);
void show_other_ships (FILE * fd, struct PLAYER *player, int star);
void show_shop_options (FILE * fd, int shop);
void show_location_option (FILE * fd, struct PLAYER *player, int site);
void show_location (FILE * fd, struct PLAYER *player, int site);
void starname_input (FILE * fd);
int sale_price (struct ITEM *item);
void show_selling_options (FILE * fd, struct PLAYER *player);
void show_system_options (FILE * fd, struct PLAYER *player, int star);
void show_general_options (FILE * fd, struct PLAYER *player);
void show_starsystem (FILE * fd, struct PLAYER *player, int star);
void show_map (FILE * fd);
void show_gif_map (FILE * fd, struct PLAYER *player);
void starnet_report (struct PLAYER *player);
void show_board (int debug);
const char * password (int key);
void generate_options (FILE * fd, struct PLAYER *player, skill_sort sort);
int star_has_ring (int star, skill_sort sort);
void retire_prophet (skill_sort skill);
int spell_valid (struct PLAYER *player, int spell);
void show_scrap_options (FILE * fd, struct PLAYER *player);
void show_merc_options (FILE * fd, struct PLAYER *player);
void dybuk_menu (FILE * fd, int player);
void show_orders (FILE *fd, struct PLAYER *player);
void show_player (FILE * fd, struct PLAYER *player);
void create_header (struct PLAYER *player);
void init_new_player (struct PLAYER *player, int sort);
void init_players ();
void reset_aliens ();
void init_aliens ();
int ship_size (struct ITEM *item);
void name_shops ();
void reset_shops ();
void init_shops ();
void reset_npcs ();
void reset_stars ();
void init_game ();
void upgrade (struct PLAYER *player);
int total_item (item_sort sort, struct ITEM *item);
int total_working_item (item_sort sort, struct ITEM *item);
void consolidate_artifacts ();
void debug (FILE * fd, struct PLAYER *player);
int protected_items (short item);
int is_wraith (struct PLAYER *player);
int is_player (struct PLAYER *player);
void new_ranking (char *p);
void press (int player, const char *text, const char *author);
int decode_starname (char *name, int current);
void remove_html (char *buffer);
int shop_owns_item (int item);
void shop_at (FILE * fd, struct PLAYER *player, int item);
void disable (FILE * fd, struct PLAYER *player, int item);
void collect (FILE * fd, struct PLAYER *player, int loc, char qualifier);
int any_gates (struct PLAYER *player, int from, int to);
int jump_dirt (struct PLAYER *player, int from, int to);
void explore (FILE * fd, struct PLAYER *player, int loc);
void cure (FILE * fd, struct PLAYER *player);
void train (FILE * fd, struct PLAYER *player, skill_sort skill);
void open_results (FILE ** fd, struct PLAYER *player);
void check_passwords (FILE * fd, struct PLAYER *player);
void fix_criminals ();
void check_allies (FILE * fd, int player);
void end_turn ();
void scrap (FILE * fd, struct PLAYER *player, int code);
void show_experience (FILE * fd, struct PLAYER *player);
void pw_train (FILE * fd, struct PLAYER *player, skill_sort sort);
void do_password_powers (FILE * fd, struct PLAYER *player);
void mudd (FILE * fd, struct PLAYER *player);
void steal_locations ();
void long_range_scan (FILE * fd, struct PLAYER *player);
void change_companion (FILE * fd, struct PLAYER *player, int value);
int ground_strength (struct PLAYER *player, location_sort sort);
void unload_medicine (FILE * fd, struct PLAYER *player);
void move_wraiths ();
void hide_systems ();
void update_power ();
int power_rating (struct PLAYER *player);
int player_load ();
void read_all_orders ();
void write_results ();
void merge_results ();
void open_times ();
void generate_times (int code);
void close_times ();
void check_items ();
void init_units ();
void update_passwds ();
void shop_stats ();
void check_flags ();
void show_rings ();
void make_web_form (FILE * fd, char *label, int code);
void make_links ();
int cmpname (const void *a, const void *b);
void sort_names ();
void make_web_pages ();
void make_secrets ();
void write_new_files ();
void make_ship_files ();
void chrcat (char *p, char c);
void write_demographics ();
const char * minister_name(int i);
void show_candidates ();
void show_plagues ();
int count_modules (struct ITEM *base, int sort);
int get_nth_module (struct ITEM *base, int sort, int n);
int jm_main ();
 
#endif 
