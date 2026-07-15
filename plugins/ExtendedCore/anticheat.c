#include "anticheat.h"
#include "peer.h"
#include <Plugin.h>
#include <Config.h>

/*
 * Lightweight shadow of ring counts. Stock Game.c already enforces
 * data_based_anticheat (gomunkulus / HP). Aggressive "inflate" kicks here
 * false-positive on Cream multi-rings and batched PLAYER_DATA updates.
 */

#define EC_HARD_RING_CAP          999u
#define EC_SUSPICIOUS_JUMP        40u  /* single PLAYER_DATA jump above this -> kick */
#define EC_RING_GRACE_TICKS       (3.0 * TICKSPERSEC)
#define EC_LOST_RING_CREDIT_TICKS (10.0 * TICKSPERSEC)

static ECPeer* ec_peer(PeerData* peer, int plugin_id)
{
	return peer ? (ECPeer*)plugin_peer_udata(peer, plugin_id) : NULL;
}

static bool ec_disconnect(PeerData* peer, const char* reason)
{
	if (!peer || !peer->server)
		return false;
	server_disconnect(peer->server, peer->peer, DR_OTHER, reason);
	return false;
}

void ec_anticheat_grant_rings(PeerData* peer, int plugin_id, uint16_t count)
{
	ECPeer* state = ec_peer(peer, plugin_id);
	if (!peer || !state)
		return;

	uint32_t rings = (uint32_t)peer->plr.rings + count;
	if (rings > EC_HARD_RING_CAP)
		rings = EC_HARD_RING_CAP;
	peer->plr.rings = (uint16_t)rings;
	state->last_rings = peer->plr.rings;
	state->pending_ring_claim = 0;
	state->ring_grace_timer = EC_RING_GRACE_TICKS;
}

void ec_anticheat_spend_rings(PeerData* peer, int plugin_id, uint16_t count)
{
	ECPeer* state = ec_peer(peer, plugin_id);
	if (!peer || !state)
		return;

	peer->plr.rings = peer->plr.rings > count ? peer->plr.rings - count : 0;
	state->last_rings = peer->plr.rings;
	state->pending_ring_claim = 0;
	state->ring_grace_timer = EC_RING_GRACE_TICKS;
}

void ec_anticheat_note_damage_ring_loss(PeerData* peer, int plugin_id)
{
	ECPeer* state = ec_peer(peer, plugin_id);
	if (!peer || !state || peer->plr.rings == 0)
		return;

	uint32_t credit = (uint32_t)state->lost_ring_credit + peer->plr.rings;
	if (credit > EC_HARD_RING_CAP)
		credit = EC_HARD_RING_CAP;
	state->lost_ring_credit = (uint16_t)credit;
	ec_anticheat_spend_rings(peer, plugin_id, peer->plr.rings);
	state->heal_grace_timer = EC_LOST_RING_CREDIT_TICKS;
}

bool ec_anticheat_on_player_data(PeerData* peer, int plugin_id)
{
	ECPeer* state = ec_peer(peer, plugin_id);
	if (!peer || !state || !peer->in_game || peer->id == peer->server->game.exe)
		return true;

	/* Respect stock toggle: if data AC is off, only track, never kick. */
	bool enforce = g_config.states.gameplay.anticheat.data_based_anticheat;

	if (peer->plr.rings > EC_HARD_RING_CAP)
		return enforce ? ec_disconnect(peer, "ring cap exceeded") : true;

	if (!state->anticheat_initialized) {
		state->anticheat_initialized = true;
		state->last_rings = peer->plr.rings;
		state->last_hp = 100;
		return true;
	}

	if (state->ring_grace_timer > 0) {
		state->last_rings = peer->plr.rings;
		state->pending_ring_claim = 0;
		return true;
	}

	if (peer->plr.rings <= state->last_rings) {
		state->last_rings = peer->plr.rings;
		state->pending_ring_claim = 0;
		return true;
	}

	uint16_t increase = (uint16_t)(peer->plr.rings - state->last_rings);

	/* Normal pickups / Cream / delayed sync: accept and resync shadow. */
	if (increase <= EC_SUSPICIOUS_JUMP) {
		state->last_rings = peer->plr.rings;
		state->pending_ring_claim = 0;
		return true;
	}

	if (enforce)
		return ec_disconnect(peer, "rings inflate");

	state->last_rings = peer->plr.rings;
	return true;
}

void ec_anticheat_tick(PeerData* peer, int plugin_id)
{
	ECPeer* state = ec_peer(peer, plugin_id);
	if (!peer || !state)
		return;

	if (state->ring_grace_timer > 0) {
		state->ring_grace_timer -= peer->server->delta;
		if (state->ring_grace_timer < 0)
			state->ring_grace_timer = 0;
	}
	if (state->heal_grace_timer > 0) {
		state->heal_grace_timer -= peer->server->delta;
		if (state->heal_grace_timer <= 0) {
			state->heal_grace_timer = 0;
			state->lost_ring_credit = 0;
		}
	}

	if (state->pending_death_timer > 0) {
		state->pending_death_timer -= peer->server->delta;
		if (state->pending_death_timer < 0)
			state->pending_death_timer = 0;
	}
}
