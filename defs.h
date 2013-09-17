#define FALSE   0
#define TRUE    ~FALSE

#define BIG_NUMBER      20000

#define MAXX 70
#define MAXY 30
#define MAX_STAR 128
#define MAX_FAKE_STAR 3
#define MAX_HAB_STAR 64

#define MAX_LOCATION    1500
#define MAX_ADVENTURE   256
#define MAX_CRIMINAL    256
#define NO_CRIMINAL     0

#define MAX_RING        8

#define MAX_OLD_PLAYER  200
#define MAX_PLAYER      200
#define MAX_ACCOUNT     600


#define MAX_RUMOUR      (MAX_PLAYER * 4)

#define MAX_OLD_ALIEN   128
#define MAX_ALIEN       256
#define MAX_SHOP        64

#define MAX_SHIP (MAX_PLAYER + MAX_ALIEN + MAX_SHOP)

#define NAME_SIZE 10

#define NOWHERE -3
#define OLYMPUS -2
#define HOLIDAY -1

#define OLYMPUS_SEEN    0x10
#define NO_LOCATION     -1

#define LOC_NONE        0
#define LOC_ADVENTURE   1
#define LOC_CRIMINAL    2
#define LOC_EXPLORABLE  (LOC_ADVENTURE | LOC_CRIMINAL)
#define LOC_RISK        4
#define LOC_POPCORN     8
#define LOC_ROGUE       16
#define LOC_HIDE        32
#define LOC_MERC        64

#define BASE_UNIT       64
#define MAX_UNIT        64

#define NOT_HOMEWORLD   -1

#define PRESIDENT       4
#define TRIBUNE         5
#define VEEP            6
#define MIN_IND         7
#define MIN_JUST        8

#define DYBUK_CANDIDATE 0
#define DYBUK_SAFE      -1
#define DYBUK_TARGET    BIG_NUMBER

#define NO_ITEM         -1
#define MAX_ITEM        15000
#define OLD_MAX_ITEM    15000

#define ITEM_DEMANDED   1
#define ITEM_OFFERED    2
#define ITEM_TARGETTED  4
#define ITEM_PROTECTED  8
#define ITEM_DEMO       16
#define ITEM_BROKEN     32
#define ITEM_IN_USE     64
#define ITEM_REALLY_IN_USE      128
#define ITEM_LUCKY      256

#define ITEM_SAVE_FLAGS (ITEM_DEMO | ITEM_BROKEN | ITEM_IN_USE)
/* flags to be saved from turn to turn */

#define ADVENTURE_TYPE(parameter) ((parameter) & 15)
#define ADVENTURE_SKILL(parameter) ((parameter) >> 6)
#define ADVENTURE_LEVEL(parameter) ((parameter) & 31)

#define BONUS_ENGINEERING       1
#define BONUS_SCIENCE           2
#define BONUS_MEDICAL           4
#define BONUS_WEAPONARY         8
#define BONUS_DIVINE            16

#define PLAYER_ALLIANCE -1
#define SHOP_ALLIANCE   -2

#define DISGRACED       1

#define FLAG_HIDE_SYSTEM        (1 << 0)
#define FLAG_BLESS_MEDICAL      (1 << 3)
#define FLAG_STIMULATE          (1 << 4)
#define FLAG_AVOID_COMBAT       (1 << 5)
#define FLAG_FORCE_COMBAT       (1 << 6)
#define FLAG_PROTECT_CREW       (1 << 7)
#define FLAG_USE_PROBE          (1 << 8)
#define FLAG_SUPER_ENGINEERING  (1 << 9)
#define FLAG_SUPER_SCIENCE      (1 << 10)
#define FLAG_SUPER_MEDICAL      (1 << 11)
#define FLAG_SUPER_WEAPONRY     (1 << 12)
#define FLAG_ATONE_ENGINEERING  (1 << 13)
#define FLAG_ATONE_SCIENCE      (1 << 14)
#define FLAG_ATONE_MEDICAL      (1 << 15)
#define FLAG_ATONE_WEAPONRY     (1 << 16)

#define GOOD_NUMBER(parameter) (((parameter) & 31) + 1)
#define RACE_NUMBER(parameter) (((parameter) >> 3) & 31)

#define SCRAP           1       /* special case trade item */
#define CHOCOLATE       2       /* special case trade item */

#define MAX_RACE        32

#define CANDIDATE       0x1
#define PROCONSUL       0x2
#define CENSORED        0x4

#define KEEP_POLITICS_FLAGS     PROCONSUL

#define PW_ENG_TRAIN    0x1
#define PW_WD_BLESS     0x2
#define PW_ID_BLESS     0x4
#define PW_ENG_EXPLORE  0x8
#define PW_SCI_TRAIN    0x10
#define PW_SN_BLESS     0x20
#define PW_CL_BLESS     0x40
#define PW_SCI_EXPLORE  0x80
#define PW_MED_TRAIN    0x100
#define PW_LS_BLESS     0x200
#define PW_SB_BLESS     0x400
#define PW_MED_EXPLORE  0x800
#define PW_WEA_TRAIN    0x1000
#define PW_SH_BLESS     0x2000
#define PW_WP_BLESS     0x4000
#define PW_WP_EXPLORE   0x8000

#define MAGIC_NO_SPELL                  0
#define MAGIC_REPORT_SOME               1
#define MAGIC_PURGE_RIVALS              2
#define MAGIC_REPORT_ALL                3
#define MAGIC_FAKE_KEY                  4
#define MAGIC_PACIFY                    5
#define MAGIC_BLESS_WARP                6
#define MAGIC_BLESS_IMPULSE             7
#define MAGIC_BLESS_SENSOR              8
#define MAGIC_BLESS_CLOAK               9
#define MAGIC_BLESS_LIFE                10
#define MAGIC_BLESS_SICKBAY             11
#define MAGIC_BLESS_SHIELD              12
#define MAGIC_BLESS_WEAPON              13
#define MAGIC_UNCURSE_WARP              14
#define MAGIC_UNCURSE_IMPULSE           15
#define MAGIC_UNCURSE_SENSOR            16
#define MAGIC_UNCURSE_CLOAK             17
#define MAGIC_UNCURSE_LIFE              18
#define MAGIC_UNCURSE_SICKBAY           19
#define MAGIC_UNCURSE_SHIELD            20
#define MAGIC_UNCURSE_WEAPON            21
#define MAGIC_CHARM_ENGINEERING         22
#define MAGIC_CHARM_SCIENCE             23
#define MAGIC_CHARM_MEDICAL             24
#define MAGIC_CHARM_WEAPONARY           25
#define MAGIC_GROUND_COMBAT             26
#define MAGIC_ENGINEERING_ENLIGHTENMENT 27
#define MAGIC_SCIENCE_ENLIGHTENMENT     28
#define MAGIC_MEDICAL_ENLIGHTENMENT     29
#define MAGIC_WEAPONRY_ENLIGHTENMENT    30
#define MAGIC_ENGINEERING_PROPHET       31
#define MAGIC_SCIENCE_PROPHET           32
#define MAGIC_MEDICAL_PROPHET           33
#define MAGIC_WEAPONRY_PROPHET          34
#define MAGIC_ENGINEERING_RETIRE        35
#define MAGIC_SCIENCE_RETIRE            36
#define MAGIC_MEDICAL_RETIRE            37
#define MAGIC_WEAPONRY_RETIRE           38
#define MAGIC_ENGINEERING_PRAISE        39
#define MAGIC_SCIENCE_PRAISE            40
#define MAGIC_MEDICAL_PRAISE            41
#define MAGIC_WEAPONRY_PRAISE           42
#define MAGIC_ENGINEERING_DENOUNCE      43
#define MAGIC_SCIENCE_DENOUNCE          44
#define MAGIC_MEDICAL_DENOUNCE          45
#define MAGIC_WEAPONRY_DENOUNCE         46

#define MAGIC_PROTECT_SHIP              47
#define MAGIC_CONCEAL_EVIL              48
#define MAGIC_PROTECT_CREW              49
#define MAGIC_BANISH_EVIL               50
#define MAGIC_RELEASE_EVIL              51

#define MAGIC_TRACE_SHIP                52
#define MAGIC_VIEW_TRACE                53
#define MAGIC_REMOVE_TRACE              54
#define MAGIC_ENGINEERING_IMPROVE       55
#define MAGIC_SCIENCE_IMPROVE           56
#define MAGIC_MEDICAL_IMPROVE           57
#define MAGIC_WEAPONRY_IMPROVE          58
#define MAGIC_ENGINEERING_COLLECT_RING  59
#define MAGIC_SCIENCE_COLLECT_RING      60
#define MAGIC_MEDICAL_COLLECT_RING      61
#define MAGIC_WEAPONRY_COLLECT_RING     62
#define MAGIC_HIDE_SYSTEM               63
#define MAGIC_AVOID_COMBAT              64
#define MAGIC_FORCE_COMBAT              65
#define MAGIC_PROTECT_FROM_COMBAT       66
#define MAGIC_REPORT_ADVENTURE          67
#define MAGIC_POWER_UP                  68
#define MAGIC_POWER_DOWN                69
#define MAGIC_BLESS_MEDICAL             70
#define MAGIC_SUMMON_POPCORN            71
#define MAGIC_STIMULATE                 72
#define MAGIC_SET_PROBE                 73
#define MAGIC_USE_PROBE                 74
#define MAGIC_CLEAR_PROBES              75
#define MAGIC_LUCKY_WARP                76
#define MAGIC_LUCKY_IMPULSE             77
#define MAGIC_LUCKY_SENSOR              78
#define MAGIC_LUCKY_CLOAK               79
#define MAGIC_LUCKY_SICKBAY             80
#define MAGIC_LUCKY_LIFESUPPORT         81
#define MAGIC_LUCKY_SHIELD              82
#define MAGIC_LUCKY_WEAPON              83
#define MAGIC_SUPER_ENGINEERING         84
#define MAGIC_SUPER_SCIENCE             85
#define MAGIC_SUPER_MEDICAL             86
#define MAGIC_SUPER_WEAPONRY            87
#define MAGIC_ATONE_ENGINEERING         88
#define MAGIC_ATONE_SCIENCE             89
#define MAGIC_ATONE_MEDICAL             90
#define MAGIC_ATONE_WEAPONRY            91
#define MAGIC_REVEAL_EVIL               92

#define GROUND_COMBAT_BIT       0x10000000
#define PROTECT_SHIP_BIT        0x20000000
#define PROTECT_CREW_BIT        0x40000000
#define BANISH_EVIL_BIT         0x80000000

#define GREATER_EVIL  0x80000000

