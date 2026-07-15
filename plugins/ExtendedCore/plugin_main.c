#include <Plugin.h>
#include <Log.h>
#include <Server.h>
#include "peer.h"
#include "accounts.h"
#include "locale.h"
#include "chat.h"
#include "skills.h"
#include "menu.h"
#include "anticheat.h"
#include "abilities.h"
#include "promo.h"

static const PluginHost* g_host = NULL;
static int g_plugin_id = -1;

static void ec_on_init(const PluginHost* host)
{
	g_host = host;
	ec_locale_init();
	ec_menu_init();
	if (!ec_accounts_init())
		Warn("ExtendedCore: failed to load accounts");
	Info("ExtendedCore: accounts, menu, abilities, anticheat ready");
}

static void ec_on_shutdown(void)
{
	ec_accounts_shutdown();
	g_host = NULL;
}

static void ec_on_peer_identity_ok(PeerData* peer)
{
	ec_auth_on_identity_ok(peer, g_host, g_plugin_id);
}

static PluginResult ec_on_chat(PeerData* peer, String* msg)
{
	return ec_chat_handle(peer, msg, g_host, g_plugin_id);
}

static PluginResult ec_on_packet(PeerData* peer, Packet* packet)
{
	/*
	 * Stock Lobby sends help/location/MOTD inside this handler.
	 * Defer our account lines to on_tick so they appear after (order 4–5).
	 */
	if (packet && packet->len > 1 && packet->buff[1] == CLIENT_LOBBY_PLAYERS_REQUEST) {
		ECPeer* ec = (ECPeer*)plugin_peer_udata(peer, g_plugin_id);
		if (ec && !ec->welcome_sent)
			ec->welcome_pending = true;
	}
	return PLUGIN_CONTINUE;
}

static void ec_on_tick(Server* server, double delta)
{
	ec_auth_tick(server, delta, g_host, g_plugin_id);
	ec_menu_tick(server, delta, g_host, g_plugin_id);
	ec_promo_tick(server, delta, g_host, g_plugin_id);
}

static void ec_on_player_data(PeerData* peer)
{
	if (ec_anticheat_on_player_data(peer, g_plugin_id))
		ec_abilities_on_player_data(peer, g_host, g_plugin_id);
}

static void ec_on_game_player_tick(Server* server, PeerData* peer)
{
	ec_anticheat_tick(peer, g_plugin_id);
	ec_abilities_player_tick(server, peer, g_host, g_plugin_id);
}

static PluginResult ec_on_player_death(PeerData* peer)
{
	return ec_abilities_on_death(peer, g_host, g_plugin_id);
}

static void ec_on_game_start(Server* server)
{
	ec_abilities_on_game_start(server, g_plugin_id);
}

static void ec_on_game_end(Server* server, int ending)
{
	ec_rewards_on_game_end(server, ending, g_plugin_id);
}

static void ec_on_results(Server* server)
{
	ec_skills_announce_results(server, g_host);
}

bool extendedcore_plugin_entry(const PluginHost* host)
{
	(void)host;

	PluginInfo info = {
		.name = "ExtendedCore",
		.version = "1.0.0",
		.api_version = PLUGIN_API_VERSION,
		.peer_data_size = sizeof(ECPeer),
		.server_data_size = sizeof(ECServer),
		.hooks = {
			.on_init = ec_on_init,
			.on_shutdown = ec_on_shutdown,
			.on_peer_identity_ok = ec_on_peer_identity_ok,
			.on_chat = ec_on_chat,
			.on_packet = ec_on_packet,
			.on_tick = ec_on_tick,
			.on_player_data = ec_on_player_data,
			.on_game_player_tick = ec_on_game_player_tick,
			.on_player_death = ec_on_player_death,
			.on_game_start = ec_on_game_start,
			.on_game_end = ec_on_game_end,
			.on_results = ec_on_results,
		}
	};

	g_plugin_id = plugin_register(&info);
	return g_plugin_id >= 0;
}

PLUGIN_EXPORT bool plugin_entry(const PluginHost* host)
{
	return extendedcore_plugin_entry(host);
}
