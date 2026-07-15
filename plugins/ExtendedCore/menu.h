#ifndef EC_MENU_H
#define EC_MENU_H

#include <stdbool.h>
#include <Plugin.h>
#include <Server.h>

void ec_menu_init(void);
bool ec_menu_handle_chat(PeerData* v, String* msg, const PluginHost* host, int plugin_id);
void ec_menu_open_main(PeerData* v, const PluginHost* host, int plugin_id);
void ec_menu_tick(Server* server, double delta, const PluginHost* host, int plugin_id);
bool ec_menu_is_muted(PeerData* v, int plugin_id);

#endif
