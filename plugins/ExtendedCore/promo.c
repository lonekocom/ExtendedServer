#include "promo.h"
#include "peer.h"
#include <stdlib.h>
#include <string.h>

#define EC_WARP_MINUTES_MIN    20
#define EC_WARP_MINUTES_MAX    45
#define EC_DISCORD_MINUTES_MIN 25
#define EC_DISCORD_MINUTES_MAX 50

static double next_interval(int min_minutes, int max_minutes)
{
	int minutes = min_minutes + rand() % (max_minutes - min_minutes + 1);
	return (double)(minutes * 60 * TICKSPERSEC);
}

void ec_promo_tick(Server* server, double delta, const PluginHost* host, int plugin_id)
{
	ECServer* state = (ECServer*)host->server_udata(server, plugin_id);
	if (!state)
		return;

	if (state->warp_hint_timer <= 0)
		state->warp_hint_timer = next_interval(EC_WARP_MINUTES_MIN, EC_WARP_MINUTES_MAX);
	if (state->discord_hint_timer <= 0)
		state->discord_hint_timer = next_interval(EC_DISCORD_MINUTES_MIN, EC_DISCORD_MINUTES_MAX);

	state->warp_hint_timer -= delta;
	state->discord_hint_timer -= delta;
	bool send_warp = state->warp_hint_timer <= 0;
	bool send_discord = state->discord_hint_timer <= 0;
	if (send_warp)
		state->warp_hint_timer = next_interval(EC_WARP_MINUTES_MIN, EC_WARP_MINUTES_MAX);
	if (send_discord)
		state->discord_hint_timer = next_interval(EC_DISCORD_MINUTES_MIN, EC_DISCORD_MINUTES_MAX);

	if (!send_warp && !send_discord)
		return;
	for (size_t i = 0; i < server->peers.capacity; ++i) {
		PeerData* peer = (PeerData*)server->peers.ptr[i];
		ECPeer* peer_state = peer ? (ECPeer*)host->peer_udata(peer, plugin_id) : NULL;
		if (!peer || !peer_state || !peer->verified || peer->id == 0)
			continue;
		if (send_warp && strcmp(peer_state->language, "ru") == 0)
			host->send_msg(server, peer->peer,
				"для стабилизации пинга рекомендуется Cloudflare WARP: https://one.one.one.one/");
		if (send_discord)
			host->send_msg(server, peer->peer,
				"join our Discord: https://discord.gg/PUDHYT3Ebv");
	}
}
