#include "abilities.h"
#include "anticheat.h"
#include "peer.h"
#include <math.h>
#include <string.h>

#define EC_GAS_LEVEL 10
#define EC_GAS_STATE 10
#define EC_GAS_COOLDOWN (60 * TICKSPERSEC)
#define EC_GAS_DURATION (10 * TICKSPERSEC)
#define EC_GAS_DRAIN (2 * TICKSPERSEC)
#define EC_CEREAL_LEVEL 5
#define EC_ORBITAL_LEVEL 5
#define EC_STONE_LEVEL 5
#define EC_STONE_DURATION (10 * TICKSPERSEC)
#define EC_STONE_COOLDOWN (60 * TICKSPERSEC)
#define EC_AURA_LEVEL 5
#define EC_AURA_DURATION (8 * TICKSPERSEC)
#define EC_AURA_PULSE (2 * TICKSPERSEC)
#define EC_RADAR_LEVEL 5
#define EC_SALLY_DECREE_LEVEL 5
#define EC_DECREE_DURATION (9 * TICKSPERSEC)
#define EC_DECREE_COOLDOWN (60 * TICKSPERSEC)
#define EC_DECREE_STUN (4 * TICKSPERSEC)
#define EC_RADAR_TRACK_BASE 0xF000u

static ECPeer* ep(PeerData* peer, const PluginHost* host, int id)
{
	return peer && host ? (ECPeer*)host->peer_udata(peer, id) : NULL;
}

static bool alive(const PeerData* p)
{
	return p && p->in_game && !(p->plr.flags & (PLAYER_DEAD | PLAYER_DEMONIZED | PLAYER_ESCAPED));
}

static bool is_survivor(const PeerData* p)
{
	return alive(p) && p->id != p->server->game.exe;
}

static bool send_sound(PeerData* peer, const PluginHost* host, uint8_t sound)
{
	Packet packet;
	PacketCreate(&packet, CLIENT_SOUND_EMIT);
	PacketWrite(&packet, packet_write16, peer->id);
	PacketWrite(&packet, packet_write8, sound);
	PacketWrite(&packet, packet_write8, 0);
	host->send_packet(peer->peer, &packet, true);
	return true;
}

static bool send_damage(PeerData* peer, const PluginHost* host, uint8_t damage, int8_t dx, int8_t dy)
{
	Packet packet;
	PacketCreate(&packet, SERVER_FORCE_DAMAGE);
	PacketWrite(&packet, packet_write8, damage);
	PacketWrite(&packet, packet_write8, (uint8_t)dx);
	PacketWrite(&packet, packet_write8, (uint8_t)dy);
	host->send_packet(peer->peer, &packet, true);
	return true;
}

static bool send_radar(PeerData* peer, const PluginHost* host, ECPeer* state, uint16_t target)
{
	Packet packet;
	uint16_t track = (uint16_t)(EC_RADAR_TRACK_BASE | ((uint16_t)peer->id << 4) | (++state->radar_track_seq & 0x0F));
	PacketCreate(&packet, SERVER_ETRACKER_STATE);
	PacketWrite(&packet, packet_write8, 0);
	PacketWrite(&packet, packet_write16, track);
	PacketWrite(&packet, packet_write16, 0);
	PacketWrite(&packet, packet_write16, 0);
	host->send_packet(peer->peer, &packet, true);
	PacketCreate(&packet, SERVER_ETRACKER_STATE);
	PacketWrite(&packet, packet_write8, 1);
	PacketWrite(&packet, packet_write16, track);
	PacketWrite(&packet, packet_write16, target);
	host->send_packet(peer->peer, &packet, true);
	state->radar_refresh_timer = 90;
	return true;
}

static bool radar_off(PeerData* peer, const PluginHost* host, ECPeer* state)
{
	state->tails_radar_on = false;
	state->sally_radar_on = false;
	state->sally_radar_timer = 0;
	return send_radar(peer, host, state, 0xFFFFu);
}

static bool activate_gas(PeerData* peer, const PluginHost* host, ECPeer* state)
{
	state->exe_gas_active = true;
	state->exe_gas_timer = EC_GAS_DURATION;
	state->exe_gas_drain_timer = 0;
	state->exe_gas_cooldown = EC_GAS_COOLDOWN;
	send_sound(peer, host, 27);
	host->broadcast_msg(peer->server, 0, "The EXE released poisonous gas!");
	return true;
}

static bool activate_decree(PeerData* sally, const PluginHost* host, int id)
{
	ECPeer* state = ep(sally, host, id);
	state->sally_decree_cooldown = EC_DECREE_COOLDOWN;
	state->sally_decree_channeling = true;
	state->sally_decree_channel_timer = EC_DECREE_DURATION;
	send_sound(sally, host, 19);

	for (size_t i = 0; i < sally->server->peers.capacity; ++i) {
		PeerData* target = (PeerData*)sally->server->peers.ptr[i];
		ECPeer* target_state = ep(target, host, id);
		if (!target || !target_state || !alive(target))
			continue;
		if (target->id == sally->server->game.exe || (target->plr.flags & PLAYER_DEMONIZED)) {
			target_state->sally_decree_stun_timer = EC_DECREE_STUN;
			target_state->sally_decree_stun_pos = target->plr.pos;
			send_damage(target, host, 0, 0, 0);
		} else {
			target_state->sally_decree_timer = EC_DECREE_DURATION;
		}
	}
	return true;
}

static bool activate_orbital(PeerData* eggman, const PluginHost* host)
{
	Server* server = eggman->server;
	Packet effect;
	PacketCreate(&effect, CLIENT_SPAWN_EFFECT);
	PacketWrite(&effect, packet_write16, (uint16_t)eggman->plr.pos.x);
	PacketWrite(&effect, packet_write16, (uint16_t)eggman->plr.pos.y);
	PacketWrite(&effect, packet_write8, 0);
	PacketWrite(&effect, packet_write8, 1);
	PacketWrite(&effect, packet_write8, 11);
	PacketWrite(&effect, packet_write8, 0);
	PacketWrite(&effect, packet_write8, 0);
	PacketWrite(&effect, packet_writefloat, 0.75f);
	host->broadcast_packet(server, &effect, true);
	send_sound(eggman, host, 3);

	for (size_t i = 0; i < server->peers.capacity; ++i) {
		PeerData* target = (PeerData*)server->peers.ptr[i];
		if (!target || !alive(target) || target->id == eggman->id)
			continue;
		if (target->id != server->game.exe && !(target->plr.flags & PLAYER_DEMONIZED))
			continue;
		if (vector2_dist(&eggman->plr.pos, &target->plr.pos) <= 120.f)
			send_damage(target, host, 15, 0, 0);
	}
	return true;
}

static bool aura_pulse(PeerData* amy, const PluginHost* host, int id)
{
	Server* server = amy->server;
	for (size_t i = 0; i < server->peers.capacity; ++i) {
		PeerData* target = (PeerData*)server->peers.ptr[i];
		ECPeer* target_state = ep(target, host, id);
		if (!target || !target_state || !alive(target) || target->id == amy->id
			|| vector2_dist(&amy->plr.pos, &target->plr.pos) > 84.f)
			continue;
		if (target->id == server->game.exe || (target->plr.flags & PLAYER_DEMONIZED)) {
			send_damage(target, host, 20, 0, 0);
			continue;
		}
		target_state->shield_active = true;
		target_state->amy_aura_shield_active = true;
		target_state->amy_aura_shield_timer = EC_AURA_DURATION;
		Packet heal;
		PacketCreate(&heal, CLIENT_PLAYER_HEAL);
		PacketWrite(&heal, packet_write16, target->id);
		PacketWrite(&heal, packet_write16, target->plr.rings);
		host->send_packet(target->peer, &heal, true);
	}
	return true;
}

void ec_abilities_on_player_data(PeerData* peer, const PluginHost* host, int id)
{
	ECPeer* state = ep(peer, host, id);
	if (!state || !alive(peer))
		return;

	uint8_t current = peer->plr.state;
	bool activated = current != state->last_state;
	if (!activated)
		return;

	if (peer->id == peer->server->game.exe && state->level >= EC_GAS_LEVEL
		&& current == EC_GAS_STATE && !state->exe_gas_active && state->exe_gas_cooldown <= 0)
		activate_gas(peer, host, state);
	else if (peer->surv_char == CH_CREAM && state->level >= EC_CEREAL_LEVEL
		&& current == EC_GAS_STATE && !state->cream_cereal_used) {
		state->cream_cereal_used = true;
		ec_anticheat_grant_rings(peer, id, peer->plr.rings < 10 ? (uint16_t)(10 - peer->plr.rings) : 1);
		send_sound(peer, host, 22);
	} else if (peer->surv_char == CH_EGGMAN && state->level >= EC_ORBITAL_LEVEL
		&& current == EC_GAS_STATE && state->orbital_strike_cooldown <= 0) {
		state->orbital_strike_cooldown = 30 * TICKSPERSEC;
		activate_orbital(peer, host);
	} else if (peer->surv_char == CH_KNUX && state->level >= EC_STONE_LEVEL
		&& current == EC_GAS_STATE && state->stone_armor_cooldown <= 0) {
		state->stone_armor_timer = EC_STONE_DURATION;
		state->stone_armor_cooldown = EC_STONE_COOLDOWN;
		send_sound(peer, host, 22);
	} else if (peer->surv_char == CH_AMY && state->level >= EC_AURA_LEVEL
		&& current == 11 && state->amy_aura_timer <= 0 && state->amy_aura_stage < 2
		&& state->amy_aura_hits >= 1 && peer->plr.rings >= 8) {
		state->amy_aura_stage++;
		state->amy_aura_timer = EC_AURA_DURATION;
		state->amy_aura_pulse_timer = 0;
		send_sound(peer, host, 22);
	} else if (peer->surv_char == CH_TAILS && state->level >= EC_RADAR_LEVEL && current == EC_GAS_STATE) {
		if (state->tails_radar_on) radar_off(peer, host, state);
		else { state->tails_radar_on = true; send_radar(peer, host, state, peer->server->game.exe); }
	} else if (peer->surv_char == CH_SALLY && state->level >= EC_RADAR_LEVEL && current == EC_GAS_STATE) {
		if (state->sally_radar_on) radar_off(peer, host, state);
		else { state->sally_radar_on = true; state->sally_radar_timer = 5 * TICKSPERSEC; send_radar(peer, host, state, peer->server->game.exe); }
		if (state->level >= EC_SALLY_DECREE_LEVEL && state->sally_decree_cooldown <= 0)
			activate_decree(peer, host, id);
	}
	state->last_state = current;
}

void ec_abilities_player_tick(Server* server, PeerData* peer, const PluginHost* host, int id)
{
	ECPeer* state = ep(peer, host, id);
	if (!state || !peer->in_game)
		return;
	double dt = server->delta;
	ec_anticheat_tick(peer, id);

	/* Amy charges her aura by attacking close to the EXE. */
	if (peer->surv_char == CH_AMY && state->level >= EC_AURA_LEVEL && peer->plr.is_attacking
		&& state->amy_aura_hits < 1 && state->amy_aura_stage < 2) {
		PeerData* exe = host->find_peer(server, server->game.exe);
		if (exe && alive(exe) && vector2_dist(&peer->plr.pos, &exe->plr.pos) <= 72.f)
			state->amy_aura_hits = 1;
	}

#define EC_DEC_TIMER(field) do { if (state->field > 0) { state->field -= dt; if (state->field < 0) state->field = 0; } } while (0)
	EC_DEC_TIMER(exe_gas_cooldown); EC_DEC_TIMER(orbital_strike_cooldown);
	EC_DEC_TIMER(stone_armor_timer); EC_DEC_TIMER(stone_armor_cooldown);
	EC_DEC_TIMER(sally_radar_timer); EC_DEC_TIMER(sally_decree_cooldown);
	EC_DEC_TIMER(sally_decree_timer); EC_DEC_TIMER(sally_decree_stun_timer);
#undef EC_DEC_TIMER

	if (state->sally_radar_on && state->sally_radar_timer <= 0)
		radar_off(peer, host, state);
	if ((state->tails_radar_on || state->sally_radar_on) && (state->radar_refresh_timer -= dt) <= 0)
		send_radar(peer, host, state, server->game.exe);

	if (state->sally_decree_channeling && (state->sally_decree_channel_timer -= dt) <= 0)
		state->sally_decree_channeling = false;

	if (state->amy_aura_timer > 0) {
		state->amy_aura_timer -= dt;
		if ((state->amy_aura_pulse_timer -= dt) <= 0) {
			state->amy_aura_pulse_timer = EC_AURA_PULSE;
			aura_pulse(peer, host, id);
		}
		if (state->amy_aura_timer <= 0)
			state->amy_aura_timer = 0;
	}

	if (state->exe_gas_active) {
		state->exe_gas_timer -= dt;
		state->exe_gas_drain_timer += dt;
		if (state->exe_gas_timer <= 0)
			state->exe_gas_active = false;
		else if (state->exe_gas_drain_timer >= EC_GAS_DRAIN) {
			state->exe_gas_drain_timer = 0;
			for (size_t i = 0; i < server->peers.capacity; ++i) {
				PeerData* target = (PeerData*)server->peers.ptr[i];
				ECPeer* ts = ep(target, host, id);
				if (is_survivor(target) && ts && ts->sally_decree_timer <= 0 && target->plr.rings == 0 && ts->gas_hits_taken++ < 2)
					send_damage(target, host, 20, 0, 0);
			}
		}
	}

	if (state->amy_aura_shield_timer > 0 && (state->amy_aura_shield_timer -= dt) <= 0) {
		state->amy_aura_shield_timer = 0;
		state->amy_aura_shield_active = false;
	}
}

PluginResult ec_abilities_on_death(PeerData* peer, const PluginHost* host, int id)
{
	ECPeer* state = ep(peer, host, id);
	if (!state || !state->shield_active && !state->amy_aura_shield_active)
		return PLUGIN_CONTINUE;

	state->shield_active = false;
	state->amy_aura_shield_active = false;
	state->amy_aura_shield_timer = 0;
	state->pending_death = false;
	send_sound(peer, host, 24);
	host->send_msg(peer->server, peer->peer, "Your shield absorbed the fatal hit.");
	return PLUGIN_HANDLED;
}

void ec_abilities_on_game_start(Server* server, int id)
{
	for (size_t i = 0; i < server->peers.capacity; ++i) {
		PeerData* peer = (PeerData*)server->peers.ptr[i];
		ECPeer* state = peer ? (ECPeer*)plugin_peer_udata(peer, id) : NULL;
		if (!state) continue;
		state->cream_cereal_used = false;
		state->exe_gas_active = false;
		state->amy_aura_stage = 0;
		state->amy_aura_hits = 0;
		state->anticheat_initialized = false;
		state->pending_death = false;
		state->tails_radar_on = state->sally_radar_on = false;
	}
}
