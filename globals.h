#ifndef GLOBALS_H
#define GLOBALS_H 1
#include "bytes.h"
#include "defs.h"
#include <stdio.h>

extern char *passwords[MAX_ACCOUNT];

extern int num_players;

/*
extern int max_alien;
extern int max_player;
extern int max_shop;
*/

extern byte board[MAXX][MAXY];

typedef enum
  { clear, asteroids, nebula, dyson } terrain_sort;

typedef enum
  { engineering, science, medical, weaponry } skill_sort;

extern const char *terrain_names[];

struct STAR
{
  short x, y, terrain, ore;
  short loc_mask;
  short hidden;
  int instability;
  int locations;
};

extern struct STAR real_stars[MAX_STAR + MAX_FAKE_STAR];

extern struct STAR *stars;

extern char *real_star_names[MAX_STAR + MAX_FAKE_STAR];

extern char **star_names;

extern uint32 public_stars[MAX_STAR / 32];
extern uint32 evil_stars[MAX_STAR / 32];

/* should have temples too */
typedef enum
  {
    none, academy, arsenal, belt, badland,
    colony, comet, corona, deep_space,
    factory, gas_giant, hall, homeworld,
    minefield, moon, near_space, ocean,
    prison, ruins, school, stargate, terminal,
    constituency,
    last_location_type
  }
  location_sort;

struct LOCATION_TYPE
{
  char *name;
  short max_number;
  short exclusion_bit;
  short max_star;
  short flags;
  skill_sort instability_skill;
  int max_instability;
  int range;
};

extern const struct LOCATION_TYPE location_types[];
struct UNIT_TYPE
{
  char *name;
  location_sort good_terrain, bad_terrain;
  int strength;
};

extern const struct UNIT_TYPE unit_types[];

typedef enum
  { regular_infantry, mobile_infantry, hover_tanks, cyber_tanks,
    rocket_artillery, orbital_lasers, jump_mechs, assault_mechs
  }
  unit_sort;

struct UNIT
{
  char name[32];
  unit_sort sort;
  int pay;
};

struct UNIT units[MAX_UNIT];

struct AD_TYPE
{
  location_sort loc;
  char *ad_name, *ad_desc;
};

extern const struct AD_TYPE ad_types[16];

extern const char *crim_names[8];

struct LOCATION
{
  int star;
  byte sort;
  byte parameter;
  byte criminal;
  byte risk;
  byte rogues;
  byte used;
  byte ring;
  byte votes;
  byte voter;
  int influence;
  int instability;
};

extern struct LOCATION locations[MAX_LOCATION];

extern int homeworlds[32];

extern int good_prices[256];

extern const char *god_names[4];

extern int prophets[4];
extern int new_prophets[4];

extern int angels;

typedef enum
  { academy_skill, school_skill, adventure_skill,
    repair_skill, maintain_skill, enlightenment_skill,
    hacking_skill
  }
  skill_source;

extern const char *skill_names[];

typedef enum
  {
    warp_drive, impulse_drive, sensor, cloak, life_support, sick_bay,
    shield, ram, gun, disruptor, laser, missile, drone, fighter,
    pod, artifact, evil_artifact
  }
  item_sort;

extern const skill_sort repairers[];

extern const char *tech_level_names[];
extern const char *range_names[];

extern int restock_tech;
extern int restock_item;
extern int restock_rogues_race;
extern int restock_rogues_skill;

extern int ministers[9];
extern int proposed_techs[7];
extern int proposed_types[7];


struct ITEM
{
  short link;
  short price;
  int magic;
  item_sort sort;
  short flags;
  byte efficiency;
  byte reliability;
  byte collection;
  int pob;
};

extern struct ITEM *items;
extern unsigned int max_item;
extern unsigned int extra_items;

extern const char *item_names[];
extern const char *short_item_names[];

struct ADVENTURE
{
  short star;
  short treasure;
  byte obscurity;               /* chance of not seeing it */
  byte parameter;
  short loc;
  byte bonus_flags;
};

extern struct ADVENTURE adventures[MAX_ADVENTURE];

typedef enum
  { flee, no_attack, make_demands, attack_if_defied, always_attack }
  diplomatic_option;

typedef enum
  { favour_fleeing, favour_engines, favour_weapons, favour_shields,
    favour_sensors, favour_cloaks
  } combat_option;

extern const char *favour_names[];

struct STRATEGY
{
  diplomatic_option dip_option;
  combat_option cbt_option;
  byte ideal_range, firing_rate;
  short demand, target, retreat;
};

extern int *sorted_names;

struct PLAYER
{
  unsigned int ship;
  int skills[4];
  short crew[4], pools[4];
  short health;                 /* in tenths of a percent */
  short last_orders;
  int energy, popcorn, popcorn_sales, popcorn_buy, popcorn_price;
  struct STRATEGY strategy;
  struct PLAYER *next;
  char name[80];
  char address[128];
  char banner[128];
  char banner_source[128];
  char x_from[128];
  int friends, enemies;
  int alliance;
  uint32 ads[MAX_ADVENTURE / 32];
  uint32 crims[MAX_CRIMINAL / 32];
  uint32 stars[MAX_STAR / 32];
  int experience[4];
  int favour[4];
  int dybuk_target, last_demon, next_demon;
  int pillage;
  int politics;
  int votes;
  int plebs, trib;
  int pollution;
  int damage;
  int freebies;
  int rings_seen, rings_held;
  int ranking;
  int torps;
  int artifacts;
  int blessings;
  int reports;                  /* bit map of visible terminals */
  int allies;
  char web_source[256];
  int away_team[4];
  int standby, hide_hunt;
  int bid, gift;
  int password;
  char powermod, newpowermod;
  short prisoner, medicine;
  short account_number;
  short last_restart;
  short star, old_star;
  byte rank;
  byte results, got_some_orders;
  byte chosen, heretic, denounced;
  uint32 magic_flags;
  short passwd_flags;
  short shields, losses, reserve, reserve_per_item, fleeing;
  /* used in combat */
  int movement;
  uint32 flags;
  byte restarting, lender, sponsor;
  int preferences;
  byte tracer, viewing_trace, companion;
  int probe;
  int countdown;
  unsigned int evil;
  int evilpedos[4];
};

struct PLAYER ships[MAX_SHIP];
//extern struct PLAYER *ships;
extern struct PLAYER *players;
extern struct PLAYER *aliens;
extern struct PLAYER *shops;
extern struct PLAYER *dybuk;

extern struct PLAYER loot;

extern int *total_votes;

struct TRADE_GOOD
{
  char *name;
  int basic_value;
};

extern const struct TRADE_GOOD goods[33];

struct POPCORN
{
  short star, evil_chained;
  short impulse_limit, sensor_limit, shield_limit;
  int last_released, last_collected, reward;
};

extern struct POPCORN popcorn;

typedef enum
  { friendly, neutral, chaotic, hostile } alien_mood;

typedef enum
  { archer, balanced, sneaky, trader, pirate } alien_style;

struct ALIEN
{
  char *name;
  char tech_level;
  alien_mood hostility;
  alien_style style;
  skill_sort religion;
  int minister;
  int wealth;
  short plague;
};

extern struct ALIEN races[MAX_RACE];

extern int next_item;
extern int battle_pollution;
extern int seed;
extern int game;
extern FILE *times, *report;
extern int active_players;
extern int average_players;
extern char *rumours[MAX_RUMOUR];
extern int popcorn_price;
extern int current_evil, next_evil;
extern int dybuk_shields, dybuk_target;
extern int current_unit, current_contract, next_unit, next_contract, next_fee;
extern int password_true, password_false, password_key;
extern int favours[4];
extern int old_favours[4];
extern int chosen[4];
extern int accuser, defendent, case_winner;

struct BONUS
{
  char *name;
  int blessing;
};

extern const struct BONUS password_bonus[16];

#endif 
