#ifndef EC_PEER_H
#define EC_PEER_H

#include <stdint.h>
#include <stdbool.h>
#include <CMath.h>

#define EC_MAX_LOGIN_LEN 32
#define EC_SALT_LEN      16
#define EC_MAX_INV_ITEMS 5
#define EC_MENU_INV_SLOTS 4

typedef enum
{
	EC_MENU_NONE = 0,
	EC_MENU_MAIN,
	EC_MENU_LEVEL,
	EC_MENU_LANG,
	EC_MENU_SHOP,
	EC_MENU_INVENTORY,
	EC_MENU_MAP_SELECT,
	EC_MENU_HELP,
	EC_MENU_CHPASS
} MenuScreen;

typedef enum
{
	EC_ITEM_NONE = 0,
	EC_ITEM_DEMON_START,
	EC_ITEM_RED_RINGS,
	EC_ITEM_SHIELD,
	EC_ITEM_MAP_SELECT,
	EC_ITEM_COUNT
} ECItemType;

typedef struct ECPeer
{
	bool     authenticated;
	char     login[EC_MAX_LOGIN_LEN];
	uint8_t  auth_op_level;

	double   login_timeout;
	double   login_warn_timer;

	uint8_t  level;
	char     title[32];
	uint32_t exp;
	uint32_t coins;
	uint32_t wins_surv;
	uint32_t wins_exe;
	int      inventory[EC_ITEM_COUNT];
	char     language[8];

	uint8_t  pending_level_up;
	double   level_up_notify_timer;
	bool     welcome_sent;
	bool     welcome_pending;

	/* Menu state belongs to the plugin, never PeerData. */
	MenuScreen menu_screen;
	bool       menu_muted;
	uint8_t    menu_chpass_step;
	char       menu_chpass_old[64];
	char       menu_chpass_new[64];
	ECItemType menu_inv_map[EC_MENU_INV_SLOTS];
	uint8_t    menu_shop_page;
	int8_t     menu_shop_map[4];
	uint8_t    menu_shop_bought_slot;
	double     menu_shop_bought_timer;
	bool       menu_shop_bought_fresh;
	uint8_t    menu_map_page;
	uint8_t    menu_help_tab;
	uint8_t    menu_help_page;
	uint8_t    menu_help_abil_sub;
	uint8_t    menu_help_abil_detail;
	uint8_t    menu_help_abil_ability;
	bool       menu_lang_from_main;
	bool       shield_active;

	/*
	 * Game abilities and anti-cheat shadows.  These deliberately mirror
	 * only plugin-owned state; Player and Server remain stock structures.
	 * Timers use the server's tick units (TICKSPERSEC).
	 */
	double   exe_gas_cooldown;
	double   exe_gas_timer;
	double   exe_gas_drain_timer;
	bool     exe_gas_active;
	double   ability_cooldown;
	uint8_t  gas_hits_taken;
	bool     gas_red_ring;

	bool     cream_cereal_used;
	double   orbital_strike_cooldown;
	double   stone_armor_timer;
	double   stone_armor_cooldown;
	bool     stone_armor_palette;

	uint8_t  amy_aura_stage;
	double   amy_aura_timer;
	double   amy_aura_pulse_timer;
	double   amy_aura_self_heal_timer;
	double   amy_aura_effect_timer;
	uint8_t  amy_aura_heart_phase;
	uint8_t  amy_aura_hits;
	bool     amy_aura_shield_active;
	double   amy_aura_shield_timer;

	bool     tails_radar_on;
	bool     sally_radar_on;
	bool     radar_debug_hint;
	uint8_t  radar_track_seq;
	double   radar_refresh_timer;
	double   sally_radar_timer;
	double   sally_decree_timer;
	double   sally_decree_channel_timer;
	bool     sally_decree_channeling;
	double   sally_decree_cooldown;
	double   sally_decree_stun_timer;
	double   sally_decree_stun_effect_timer;
	Vector2  sally_decree_stun_pos;
	bool     sally_decree_phantom_ring;

	/* Authoritative values observed by the plugin after PLAYER_DATA. */
	uint16_t last_rings;
	bool     anticheat_initialized;
	uint16_t pending_ring_claim;
	uint16_t lost_ring_credit;
	uint8_t  last_hp;
	double   heal_cooldown;
	uint16_t heal_rings;
	bool     pending_death;
	uint8_t  pending_death_rtimes;
	double   pending_death_timer;
	double   ring_grace_timer;
	double   heal_grace_timer;
	uint8_t  last_state;
	uint8_t  last_index;

	/* Kept here for plugin-owned chat flood handling, if enabled later. */
	unsigned int chat_flood_count;
	double       chat_flood_window_start;
	bool         chat_flood_kicked;
	bool         flood_exempt;
} ECPeer;

typedef struct ECServer
{
	bool     demon_start_used;
	uint16_t demon_start_player;
	bool     red_ring_boost;
	double   warp_hint_timer;
	double   discord_hint_timer;
} ECServer;

#endif
