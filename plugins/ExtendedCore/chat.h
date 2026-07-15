#ifndef EC_CHAT_H
#define EC_CHAT_H

#include <Plugin.h>
#include <Server.h>

PluginResult ec_chat_handle(PeerData* peer, String* msg, const PluginHost* host, int plugin_id);
void         ec_auth_on_identity_ok(PeerData* peer, const PluginHost* host, int plugin_id);
void         ec_auth_send_welcome(PeerData* peer, const PluginHost* host, int plugin_id);
void         ec_auth_tick(Server* server, double delta, const PluginHost* host, int plugin_id);
void         ec_rewards_on_game_end(Server* server, int ending, int plugin_id);

#endif
