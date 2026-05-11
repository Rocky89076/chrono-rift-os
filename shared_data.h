#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_PLAYERS      4
#define MAX_ENEMIES      10
#define INVENTORY_SIZE   20
#define MAX_LOGS         15
#define MAX_ACTION_QUEUE 32

struct LogMessage { char message[64]; };

typedef enum {
    WEAPON_NONE = 0,
    SOLAR_CORE,
    LUNAR_BLADE,
    ECLIPSE_RELIC,
    IRON_HALBERD,
    VENOM_DAGGER,
    THUNDERSTAFF,
    OBSIDIAN_AXE,
    FROSTBOW,
    SPLINTER_STICK
} WeaponType;

/*
 * IPC action queue: human_process writes attack/exhaust requests; the Arbiter
 * is the sole process that resolves them against enemy/player state.  This
 * enforces the isolation rule — human threads never directly modify enemy state.
 */
typedef struct {
    bool valid;
    bool target_is_player; /* true → target is in players[], false → enemies[] */
    bool is_exhaust;        /* true → drain stamina; false → deal HP damage     */
    int  actor_player_id;   /* index into players[]                              */
    int  target_id;         /* index into enemies[] or players[]                 */
    int  damage;
    WeaponType weapon;      /* WEAPON_NONE for base strike                       */
} ActionRequest;

typedef struct {
    int id;
    char name[32];
    int hp;
    int max_hp;
    int damage;
    int speed;
    int stamina;
    int max_stamina;
    bool is_alive;
    bool is_stunned;
    int stun_duration;
    bool is_taking_turn;
    bool is_equipping;
    int  gui_input;
    bool is_active_on_field;
    int  field_slot;
    int  controller_id;

    WeaponType inventory[INVENTORY_SIZE];
    pthread_t  thread_id;
} EntityStats;

inline const struct {
    int size;
    int damage;
    const char* name;
} WEAPON_STATS[] = {
    {0,  0,  "None"},
    {10, 95, "Solar Core"},
    {10, 90, "Lunar Blade"},
    {0,  0,  "Eclipse Relic"},
    {7,  55, "Iron Halberd"},
    {4,  30, "Venom Dagger"},
    {6,  50, "Thunderstaff"},
    {5,  45, "Obsidian Axe"},
    {6,  48, "Frostbow"},
    {2,  12, "Splinter Stick"}
};

typedef struct {
    sem_t           memory_mutex;
    pthread_mutex_t turn_mutex;    /* serialises one player OR one NPC action at a time */
    pthread_mutex_t floor_mutex;
    WeaponType      floor_weapon;

    EntityStats players[MAX_PLAYERS];
    EntityStats enemies[MAX_ENEMIES];
    int active_player_count;
    int active_enemy_count;
    int total_enemies_killed;

    /* Inventory long-term storage (simulates disk/secondary memory) */
    WeaponType long_term_storage[MAX_PLAYERS][50];
    int        storage_count[MAX_PLAYERS];

    /* Legendary artifact mutexes (deadlock demo resources) */
    pthread_mutex_t solar_core_mutex;
    pthread_mutex_t lunar_blade_mutex;
    pthread_mutex_t eclipse_relic_mutex;

    int solar_core_holder_id;
    int lunar_blade_holder_id;
    int eclipse_relic_holder_id;

    int solar_core_waiting_id;
    int lunar_blade_waiting_id;
    int eclipse_relic_waiting_id;

    int  target_stun_id;
    bool force_release[MAX_PLAYERS + MAX_ENEMIES];
    bool ultimate_active;

    /* Eclipse Relic is locked behind this flag; Arbiter sets it mid-battle */
    bool eclipse_relic_available;

    pid_t arbiter_pid;
    pid_t human_process_pids[2];
    pid_t npc_process_pid;

    /* Action queue: circular FIFO written by human_process, drained by Arbiter */
    ActionRequest action_queue[MAX_ACTION_QUEUE];
    int action_queue_head;
    int action_queue_tail;
    int action_queue_count;

    /* Action log circular buffer */
    LogMessage action_log[MAX_LOGS];
    int log_head;
    int log_count;

    /* Pause / game-flow flags */
    bool is_paused;
    bool game_over;
    int  custom_hero_hp_mod;   /* percentage, default 100 */
    int  custom_enemy_hp_mod;  /* percentage, default 100 */
    int  custom_enemy_count;   /* absolute,   default random 2-9 */
    int  custom_speed_mod;     /* percentage, default 100 */
    bool custom_random_enemies;
    bool difficulty_selected;
    int  difficulty_level;
    bool battle_started;
    bool mode_selected;
    int  game_mode;
    bool return_to_menu;
    int  match_result; /* 0=None 1=P1Wins 2=P2Wins 3=HeroesWin 4=LavosWins */
} GameState;

#endif
