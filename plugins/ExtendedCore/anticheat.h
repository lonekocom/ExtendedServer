#ifndef EC_ANTICHEAT_H
#define EC_ANTICHEAT_H

#include <stdbool.h>
#include <Server.h>

/* Returns false only after it has disconnected the peer. */
bool ec_anticheat_on_player_data(PeerData* peer, int plugin_id);
void ec_anticheat_tick(PeerData* peer, int plugin_id);

/* Ability code uses these to keep its server-authoritative ring shadow valid. */
void ec_anticheat_grant_rings(PeerData* peer, int plugin_id, uint16_t count);
void ec_anticheat_spend_rings(PeerData* peer, int plugin_id, uint16_t count);
void ec_anticheat_note_damage_ring_loss(PeerData* peer, int plugin_id);

#endif
