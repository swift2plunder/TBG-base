#include "defs.h"
#include "globals.h"

char *passwords[MAX_ACCOUNT];

int num_players;

/*
int max_alien;
int max_player;
int max_shop;
*/

byte board[MAXX][MAXY];

/* Option flags and variables.  These are initialized in parse_opt.  */

char *desired_directory;	/* --directory=DIR */
int want_quiet;			/* --quiet, --silent */
int want_verbose;		/* --verbose */
int really_send;		/* --dry-run */
int send_email;
char *webroot;
char *gameroot;
int turn;
unsigned int session_id;
char *server;

int next_item = 1;
int battle_pollution = 0;
int seed;
int game = 1;
FILE *times, *report;
int active_players;
int average_players;
char *rumours[MAX_RUMOUR];
int current_evil, next_evil = -1;
int dybuk_shields, dybuk_target;
int current_unit, current_contract, next_unit, next_contract, next_fee = 15;
int password_true, password_false, password_key;
int favours[4] = { 0, 0, 0, 0 };
int old_favours[4];
int chosen[4] = { 0, 0, 0, 0 };
int accuser, defendent, case_winner;

//struct PLAYER ships[MAX_SHIP];
struct PLAYER *players = ships;
struct PLAYER *aliens = ships + MAX_PLAYER;
struct PLAYER *shops = ships + MAX_PLAYER + MAX_ALIEN;
struct PLAYER *dybuk = ships + MAX_PLAYER + MAX_ALIEN - 1;

/*
struct PLAYER *ships;
struct PLAYER *players;
struct PLAYER *aliens;
struct PLAYER *shops;
struct PLAYER *dybuk;
*/
struct PLAYER loot;


int *total_votes;

const char *terrain_names[] =
  { "Impenetrable", "Busy", "Noisy", "Mixed", "Open", "Clear", "Empty" };

const char *range_names[] =
  { "Adjacent", "Close", "Short", "Medium", "Long", "Distant", "Remote" };


char *real_star_names[MAX_STAR + MAX_FAKE_STAR] = {
  "Oblivion",
  "Olympus",
  "Nowhere",
  "Magrathea",
  "Adhara",
  "Alcor",
  "Aldebaran",
  "Algol",
  "Alioth",
  "Alnitak",
  "Alphard",
  "Altair",
  "Antares",
  "Arcturus",
  "Aurigae",
  "Barnard",
  "Betelgeuse",
  "Bootis",
  "Canis",
  "Canopus",
  "Capella",
  "Caph",
  "Castor",
  "Centauri",
  "Cephei",
  "Ceti",
  "Crucis",
  "Cygni",
  "Deneb",
  "Diphda",
  "Draconis",
  "Eridani",
  "Fomalhaut",
  "Hamal",
  "Hydrae",
  "Indi",
  "Kapetyn",
  "Kochab",
  "Kruger",
  "Lalande",
  "Lupi",
  "Lyrae",
  "Markab",
  "Merak",
  "Mira",
  "Mirfak",
  "Mizar",
  "Ophiuchi",
  "Pherda",
  "Polaris",
  "Pollux",
  "Procyon",
  "Rastaban",
  "Regulus",
  "Rigel",
  "Ross",
  "Sadir",
  "Schedar",
  "Scorpii",
  "Sirius",
  "Spica",
  "Tauri",
  "Thuban",
  "Vega",
  "Wezen",
  "Wolf",
  "Zosca",
  "Bug Star",
};

char **star_names = real_star_names + MAX_FAKE_STAR;

int prophets[4] = { -1, -1, -1, -1 };
int new_prophets[4] = { 0, 0, 0, 0 };

int angels = 0;

struct STAR
real_stars[MAX_STAR + MAX_FAKE_STAR] =
  {
    {
      0, 0, clear, 0, 0},
    {
      MAXX / 2, MAXY / 2, dyson, 0, 0},
    {
      0, 0, clear, 0, 0},};

struct STAR *stars = real_stars + MAX_FAKE_STAR;

uint32 public_stars[MAX_STAR / 32];
uint32 evil_stars[MAX_STAR / 32];

struct LOCATION locations[MAX_LOCATION];

const struct LOCATION_TYPE location_types[] =
  {
    { "none", 0, 0, 0, LOC_NONE, 0, 0, 0 },
    { "academy", 4, 1, MAX_HAB_STAR, LOC_NONE, medical, 128, 3 },
    { "arsenal", 16, 2, MAX_STAR, LOC_NONE, weaponry, 112, 0 },
    { "belt", 32, 4, MAX_STAR, LOC_ADVENTURE | LOC_CRIMINAL, engineering, 96, 0 },
    { "badland", 32, 0, MAX_HAB_STAR, LOC_RISK | LOC_ROGUE | LOC_MERC,
      medical, 96, 1 },
    { "colony", 256, 0, MAX_HAB_STAR, LOC_CRIMINAL, medical, 64, 5 },
    { "comet cloud", 64, 0, MAX_STAR, LOC_ADVENTURE, engineering, 96, 0},
    { "coronasphere", 96, 8, MAX_STAR, LOC_RISK | LOC_HIDE, science, 96, 0 },
    { "deep space", 96, 0, MAX_STAR, LOC_ADVENTURE | LOC_POPCORN,
      science, 96, 6 },
    { "factory", 32, 16, MAX_HAB_STAR, LOC_CRIMINAL | LOC_MERC, medical, 112, 2 },
    { "gas giant", 96, 0, MAX_STAR, LOC_RISK | LOC_ROGUE | LOC_HIDE,
      engineering, 96, 1 },
    { "hall", 32, 32, MAX_HAB_STAR, LOC_CRIMINAL, medical, 112, 6 },
    { "homeworld", 32, 64, MAX_HAB_STAR, LOC_CRIMINAL, medical, 128, 1 },
    { "minefield", 32, 0, MAX_STAR, LOC_RISK | LOC_HIDE, weaponry, 96, 0 },
    { "moon", 96, 0, MAX_STAR, LOC_ADVENTURE | LOC_CRIMINAL, engineering, 96, 1 },
    { "near space", 96, 0, MAX_STAR, LOC_ADVENTURE | LOC_POPCORN,
      science, 96, 6 },
    { "ocean", 32, 0, MAX_HAB_STAR, LOC_ADVENTURE | LOC_CRIMINAL | LOC_MERC,
      medical, 112, 0},
    { "prison", 16, 256, MAX_STAR, LOC_NONE, weaponry, 128, 3 },
    { "ruins", 96, 0, MAX_STAR, LOC_ADVENTURE | LOC_MERC, engineering, 96, 4 },
    { "school", 16, 512, MAX_HAB_STAR, LOC_NONE, medical, 128, 6 },
    { "stargate", 64, 0, MAX_STAR, LOC_NONE, science, 96, 2 },
    { "terminal", 32, 1024, MAX_STAR, LOC_NONE, science, 112, 6 },
    { "Bug Location", 0, 0, 0, LOC_NONE, 0, 0 }
    ,};

const struct UNIT_TYPE
unit_types[8] =
  {
    { "Regular Infantry", ruins, badland, 6 },
    { "Mobile Infantry", ruins, badland, 12 },
    { "Hover Tanks", ocean, ruins, 8 },
    { "Cyber Tanks", ocean, ruins, 16 },
    { "Rocket Artillery", badland, factory, 10 },
    { "Orbital Lasers", badland, factory, 20 },
    { "Jump 'Mechs", factory, ocean, 12 },
    { "Assault 'Mechs", factory, ocean, 24 },
  };
int homeworlds[32];

int good_prices[256];

int ministers[9];
int proposed_techs[7];
int proposed_types[7];

int restock_tech = 1;
int restock_item = warp_drive;
int restock_rogues_race;
int restock_rogues_skill;

struct ITEM *items;
unsigned int max_item;
unsigned int extra_items = 0;

struct ADVENTURE adventures[MAX_ADVENTURE];

int *sorted_names;

struct POPCORN popcorn;

struct ALIEN
races[MAX_RACE] =
  {
    {
      "Ape", 1, neutral, balanced, engineering, TRIBUNE},
    {
      "Basilisk", 1, neutral, balanced, science, TRIBUNE},
    {
      "Beholder", 2, neutral, balanced, medical, medical},
    {
      "Centaur", 3, friendly, pirate, science, MIN_JUST},
    {
      "Djinni", 3, neutral, balanced, science, science},
    {
      "Ettercap", 5, friendly, archer, medical, MIN_JUST},
    {
      "Formian", 2, neutral, trader, engineering, MIN_IND},
    {
      "Gargoyle", 3, neutral, balanced, medical, medical},
    {
      "Kobold", 3, neutral, trader, medical, MIN_IND},
    {
      "Lammasu", 2, neutral, pirate, engineering, engineering},
    {
      "Lycanthrope", 3, neutral, balanced, engineering, engineering},
    {
      "Mummy", 3, neutral, trader, science, science},
    {
      "Orc", 3, neutral, balanced, science, science},
    {
      "Dragon", 3, neutral, balanced, science, science},
    {
      "Rakshasa", 2, neutral, trader, medical, MIN_IND},
    {
      "Roc", 4, friendly, trader, medical, MIN_JUST},
    {
      "Salamander", 2, neutral, balanced, medical, medical},
    {
      "Skeleton", 2, neutral, balanced, medical, medical},
    {
      "Sprite", 1, neutral, sneaky, engineering, TRIBUNE},
    {
      "Toad", 3, friendly, pirate, weaponry, MIN_JUST},
    {
      "Treant", 4, neutral, pirate, engineering, engineering},
    {
      "Unicorn", 3, neutral, trader, science, MIN_IND},
    {
      "Vampire", 2, neutral, balanced, science, VEEP},
    {
      "Wight", 1, neutral, balanced, engineering, TRIBUNE},
    {
      "Zombie", 3, neutral, balanced, engineering, engineering},
    {
      "Harpy", 2, chaotic, balanced, weaponry, weaponry},
    {
      "Titan", 2, chaotic, balanced, weaponry, weaponry},
    {
      "Wyvern", 1, chaotic, balanced, weaponry, weaponry},
    {
      "Gnome", 4, hostile, sneaky, weaponry, weaponry},
    {
      "Satyr", 2, hostile, sneaky, weaponry, VEEP},
    {
      "Sphinx", 3, hostile, pirate, weaponry, VEEP},
    {
      "Bugbear", 3, hostile, sneaky, weaponry, VEEP},};

const struct AD_TYPE
ad_types[16] =
  {
    { near_space, "Near Space Adventure 1",
      "A malfunctioning satellite" },
    { moon, "Moon Adventure 1",
      "Strange signals from underground caverns" },
    { belt, "Asteroid Belt Adventure",
      "Unknown radiation emissions" },
    { ocean, "Ocean Adventure",
      "Strange behaviour by the local flora and fauna" },
    { deep_space, "Deep Space Adventure 1",
      "A drifting alien hulk" },
    { comet, "Comet Cloud Adventure 1",
      "Curious gases in a comet's core" },
    { near_space, "Near Space Adventure 2",
      "An ancient supply buoy back in shipping lanes" },
    { ruins, "Ancient Civilisation Adventure 1",
      "Bizarre rumours from an archeological survey" },
    { moon, "Moon Adventure 2",
      "Panic from lunar miners" },
    { deep_space, "Deep Space Adventure 2",
      "An unknown alien attacking deep space probes" },
    { near_space, "Near Space Adventure 3",
      "Interesting ionisation field" },
    { ruins, "Ancient Civilisation Adventure 2",
      "Cave-in reactivates old machinery" },
    { moon, "Moon Adventure 3",
      "Thermal anomalies detected on frozen outer moon" },
    { comet, "Comet Cloud Adventure 2",
      "Lost ships traced to specific comet" },
    { deep_space, "Deep Space Adventure 3",
      "Worm-hole to another quadrant" },
    { ruins, "Ancient Civilisation Adventure 3",
      "Classical statuary behaving oddly" },
  };
/* total risk locations 256, total criminal locations 512,
   total adventures 512 */

const char *crim_names[8] = {
  "No Criminal",
  "Heavy",
  "Assassin",
  "Courier",
  "Gangster",
  "Boss",
  "Mastermind",
  "Godparent"
};

const char *god_names[4] = {
  "The Mighty One",
  "The Wise One",
  "The Merciful One",
  "The Fierce One"
};

const char *skill_names[] = { "Engineering", "Science", "Medical", "Weaponry" };

const skill_sort repairers[] =
  { engineering, engineering, science, science, medical, medical,
    weaponry, weaponry, weaponry, weaponry, weaponry, weaponry,
    weaponry, weaponry, engineering, -1
  };

const char *tech_level_names[] = {
  "None", "Primitive", "Basic", "Mediocre",
  "Advanced", "Exotic", "Magic" };


const char *item_names[] = {
  "Warp Drive",
  "Impulse Drive",
  "Sensor",
  "Cloak",
  "Life Support",
  "Sickbay",
  "Shield",
  "Ram", "Gun", "Disruptor", "Laser", "Missile", "Drone", "Fighter",
  "Pod", "Artifact",
};

const char *short_item_names[] = {
  "Wd", "Id", "Sn", "Cl", "Ls", "Sb", "Sh", "Wp",
};

const struct TRADE_GOOD goods[33] =
  {
    { "Empty", 0 },
    { "Scrap", 25 },
    { "Chocolate", 50 },
    { "Cookies", 50 },
    { "Cuff Links", 50 },
    { "Boots", 75 },
    { "Jump Ropes", 75 },
    { "Starlight Mints", 75 },
    { "Kittens", 75 },
    { "Pies", 100 },
    { "Ronin Ale", 100 },
    { "Stamps", 100 },
    { "Gummy Worms", 100 },
    { "Snow Angels", 100 },
    { "Groucho Glasses", 100 },
    { "Beatles Memorabilia", 100 },
    { "Tables", 125 },
    { "Ted Talks", 125 },
    { "Hiccup Cures", 125 },
    { "Open Document Templates", 150 },
    { "Saxophones", 150 },
    { "Tarot Cards", 200 },
    { "Cell Phones", 200 },
    { "Magic Beans", 200 },
    { "Old Dogs", 250 },
    { "Modern Art", 250 },
    { "Deques", 250 },
    { "Replicator Rations", 250 },
    { "Stuxnet Thumb Drives (!)", 400 },
    { "Bootleg MP3s (!)", 400 },
    { "Shurikens (!)", 400 },
    { "Special Brownies (!)", 500 },
    { "AOL Disks (!)", 500 },
  };

const struct BONUS password_bonus[16] =
  {
    { "Free crew training for Engineering crew", 0 },
    { "Blessing for Warp Drives", 0x10000 << warp_drive },
    { "Blessing for Impulse Drives", 0x10000 << impulse_drive },
    { "Engineering Adventure Detection", 0 },
    { "Free crew training for Science crew", 0 },
    { "Blessing for Sensors", 0x10000 << sensor },
    { "Blessing for Cloaks", 0x10000 << cloak },
    { "Science Adventure Detection", 0 },
    { "Free crew training for Medical crew", 0 },
    { "Blessing for Life Support", 0x10000 << life_support },
    { "Blessing for Sickbays", 0x10000 << sick_bay },
    { "Medical Adventure Detection", 0 },
    { "Free crew training for Weaponry crew", 0 },
    { "Blessing for Shields", 0x10000 << shield },
    { "Blessing for Weapons", 0x10000 << ram },
    { "Weaponry Adventure Detection", 0}
    ,};

const char *favour_names[] =
  { "fleeing", "engines", "weapons", "shields",
    "sensors", "cloaks"
};
