#ifndef EC_ABILITIES_H
#define EC_ABILITIES_H

#include <Plugin.h>
#include <Server.h>

void ec_abilities_on_player_data(PeerData* peer, const PluginHost* host, int plugin_id);
void ec_abilities_player_tick(Server* server, PeerData* peer, const PluginHost* host, int plugin_id);
PluginResult ec_abilities_on_death(PeerData* peer, const PluginHost* host, int plugin_id);
void ec_abilities_on_game_start(Server* server, int plugin_id);

#endif
