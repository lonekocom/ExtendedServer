#ifndef EC_SKILLS_H
#define EC_SKILLS_H

#include <Plugin.h>
#include <Server.h>
#include <stddef.h>

int ec_calculate_skill_rating(const PeerData* data, const Server* server,
	char* out_prefix, size_t prefix_size);

void ec_skills_announce_results(Server* server, const PluginHost* host);

#endif
