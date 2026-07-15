#ifndef PLUGIN_H
#define PLUGIN_H

/*
 * ExtendedServer plugin API
 *
 * Plugins register hooks and optional per-peer / per-server userdata.
 * Hook return values:
 *   PLUGIN_CONTINUE — stock logic (and later plugins) keep running
 *   PLUGIN_HANDLED  — stop the chain; stock handler is skipped where applicable
 *
 * Static plugins: compile in and call plugin_register() from their entry.
 * Dynamic plugins: export plugin_entry(const PluginHost*) from a .dll/.so
 * loaded from the plugins/ directory when PLUGIN_DYNAMIC_LOADING is enabled.
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <Packet.h>

struct Server;
struct PeerData;

#define PLUGIN_API_VERSION 2
#define PLUGIN_MAX         16

typedef enum
{
	PLUGIN_CONTINUE = 0,
	PLUGIN_HANDLED  = 1
} PluginResult;

typedef struct PluginHost
{
	int api_version;

	bool (*send_msg)(struct Server* server, ENetPeer* peer, const char* message);
	bool (*broadcast_msg)(struct Server* server, uint16_t sender, const char* message);
	bool (*send_packet)(ENetPeer* peer, Packet* packet, bool reliable);
	bool (*broadcast_packet)(struct Server* server, Packet* packet, bool reliable);
	bool (*broadcast_packet_ex)(struct Server* server, Packet* packet, bool reliable, uint16_t ignore);

	struct PeerData* (*find_peer)(struct Server* server, uint16_t id);
	int  (*peer_count)(struct Server* server);
	int  (*ingame_count)(struct Server* server);

	void* (*peer_udata)(struct PeerData* peer, int plugin_id);
	void* (*server_udata)(struct Server* server, int plugin_id);

	bool (*disconnect)(struct Server* server, ENetPeer* peer, int reason, const char* text);

	struct Server* (*get_server)(int index);
	int  (*server_count)(void);
} PluginHost;

typedef struct PluginHooks
{
	void (*on_init)(const PluginHost* host);
	void (*on_shutdown)(void);

	void (*on_server_created)(struct Server* server);
	void (*on_server_destroyed)(struct Server* server);

	void (*on_peer_connect)(struct PeerData* peer);
	void (*on_peer_disconnect)(struct PeerData* peer);

	/* After identity fields are filled, before stock join. HANDLED skips peer_identity_process. */
	PluginResult (*on_peer_identity)(struct PeerData* peer);
	void (*on_peer_identity_ok)(struct PeerData* peer);

	void (*on_peer_join)(struct PeerData* peer);
	void (*on_peer_leave)(struct PeerData* peer);

	/* Before stock state packet handler. HANDLED skips stock. */
	PluginResult (*on_packet)(struct PeerData* peer, Packet* packet);

	/* Lobby/results chat, after length check. HANDLED skips commands + broadcast. */
	PluginResult (*on_chat)(struct PeerData* peer, String* msg);

	/* Called when stock server_cmd_handle does not recognize the command. HANDLED = consumed. */
	PluginResult (*on_command)(struct PeerData* peer, unsigned long hash, String* msg);

	void (*on_tick)(struct Server* server, double delta);
	void (*on_state_change)(struct Server* server, int old_state, int new_state);

	void (*on_game_start)(struct Server* server);
	void (*on_game_end)(struct Server* server, int ending);
	void (*on_results)(struct Server* server);

	/* After stock CLIENT_PLAYER_DATA applied position/rings/state. */
	void (*on_player_data)(struct PeerData* peer);

	/* Per-peer tick while ST_GAME is running (after game_player_tick internals). */
	void (*on_game_player_tick)(struct Server* server, struct PeerData* peer);

	/* Before applying death. HANDLED = cancel death (e.g. shield). */
	PluginResult (*on_player_death)(struct PeerData* peer);
} PluginHooks;

typedef struct PluginInfo
{
	const char*  name;
	const char*  version;
	int          api_version;
	size_t       peer_data_size;
	size_t       server_data_size;
	PluginHooks  hooks;
} PluginInfo;

#ifdef _WIN32
#define PLUGIN_EXPORT __declspec(dllexport)
#else
#define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

/* Dynamic plugin entry signature */
typedef bool (*PluginEntryFn)(const PluginHost* host);

/* Register a plugin. Returns plugin id [0, PLUGIN_MAX) or -1. */
int  plugin_register(const PluginInfo* info);

void* plugin_peer_udata(struct PeerData* peer, int plugin_id);
void* plugin_server_udata(struct Server* server, int plugin_id);

/* Core lifecycle (called by the server, not by plugins) */
bool plugins_init(void);
void plugins_shutdown(void);

void plugins_server_created(struct Server* server);
void plugins_server_destroyed(struct Server* server);

void plugins_peer_connect(struct PeerData* peer);
void plugins_peer_disconnect(struct PeerData* peer);

PluginResult plugins_peer_identity(struct PeerData* peer);
void         plugins_peer_identity_ok(struct PeerData* peer);

void plugins_peer_join(struct PeerData* peer);
void plugins_peer_leave(struct PeerData* peer);

PluginResult plugins_packet(struct PeerData* peer, Packet* packet);
PluginResult plugins_chat(struct PeerData* peer, String* msg);
PluginResult plugins_command(struct PeerData* peer, unsigned long hash, String* msg);

void plugins_tick(struct Server* server, double delta);
void plugins_state_change(struct Server* server, int old_state, int new_state);

void plugins_game_start(struct Server* server);
void plugins_game_end(struct Server* server, int ending);
void plugins_results(struct Server* server);

void         plugins_player_data(struct PeerData* peer);
void         plugins_game_player_tick(struct Server* server, struct PeerData* peer);
PluginResult plugins_player_death(struct PeerData* peer);

#endif
