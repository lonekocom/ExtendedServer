#include "skills.h"
#include <Plugin.h>
#include <Player.h>
#include <stdio.h>
#include <string.h>

static char get_grade(int score)
{
	if (score >= 3000) return 'p';
	if (score >= 2500) return 's';
	if (score >= 2000) return 'a';
	if (score >= 1500) return 'b';
	if (score >= 1000) return 'c';
	if (score >= 500)  return 'd';
	return 'f';
}

static const char* get_color_str(int score)
{
	if (score >= 3000) return "&";
	if (score >= 2500) return "\xE2\x84\x96";
	if (score >= 2000) return "@";
	if (score >= 1500) return "/";
	if (score >= 1000) return "`";
	if (score >= 500)  return "~";
	return "|";
}

static void make_prefix(char* out, size_t size, int score)
{
	char grade = get_grade(score);
	const char* color = get_color_str(score);
	snprintf(out, size, "%s(%c %d)~", color, grade, score);
}

int ec_calculate_skill_rating(const PeerData* data, const Server* server,
	char* out_prefix, size_t prefix_size)
{
	int score = 0;
	int is_killer = (data->plr.flags & PLAYER_KILLER) || (data->id == server->game.exe);
	int is_demon = (data->plr.flags & PLAYER_DEMONIZED);

	if (!is_killer && !is_demon)
	{
		if (data->plr.flags & PLAYER_LEFT)
		{
			make_prefix(out_prefix, prefix_size, 0);
			return 0;
		}
		if ((data->plr.flags & PLAYER_DEAD) && !is_demon)
		{
			make_prefix(out_prefix, prefix_size, 0);
			return 0;
		}

		int rings_score = data->plr.stats.rings * 10;
		int danger_seconds = (int)(data->plr.stats.danger_time / 60.0);
		int danger_score = (danger_seconds / 10) * 200;
		int damage_penalty = data->plr.stats.damage_taken * (-200);
		int heal_score = data->plr.stats.hp_restored * 5;
		int stun_score = data->plr.stats.stuns * 50;

		score = rings_score + danger_score + damage_penalty + heal_score + stun_score;
		if (score < 0)
			score = 0;

		double modifier = 1.0;
		if (data->plr.flags & PLAYER_ESCAPED)
			modifier = 1.4;
		else if (!(data->plr.flags & PLAYER_DEAD))
			modifier = 0.8;
		score = (int)(score * modifier);

		make_prefix(out_prefix, prefix_size, score);
		return score;
	}

	if (is_killer || is_demon)
	{
		score = data->plr.stats.kills * 200 + data->plr.stats.damage * 2;

		int dead_not_escaped = 0, alive_not_escaped = 0, escaped = 0;
		for (size_t i = 0; i < server->peers.capacity; i++)
		{
			const PeerData* p = (const PeerData*)server->peers.ptr[i];
			if (!p || !p->in_game || p->id == data->id)
				continue;
			if (p->plr.flags & PLAYER_DEAD)
				dead_not_escaped++;
			else if (p->plr.flags & PLAYER_ESCAPED)
				escaped++;
			else
				alive_not_escaped++;
		}
		for (size_t i = 0; i < server->game.left.capacity; i++)
		{
			const PeerData* p = (const PeerData*)server->game.left.ptr[i];
			if (!p || !p->in_game)
				continue;
			dead_not_escaped++;
		}

		int total = dead_not_escaped + alive_not_escaped + escaped;
		if (total > 0)
		{
			float multiplier = 1.0f
				+ (dead_not_escaped * 1.0f) / total
				+ (alive_not_escaped * 0.4f) / total
				- (escaped * 0.3f) / total;
			score = (int)(score * multiplier);
		}

		if (is_demon && score > 0)
			score = (int)(score * 0.9);

		make_prefix(out_prefix, prefix_size, score);
		return score;
	}

	make_prefix(out_prefix, prefix_size, 0);
	return 0;
}

void ec_skills_announce_results(Server* server, const PluginHost* host)
{
	if (!server || !host || !host->send_msg)
		return;

	for (size_t i = 0; i < server->peers.capacity; i++)
	{
		PeerData* v = (PeerData*)server->peers.ptr[i];
		if (!v || !v->in_game)
			continue;

		char prefix[64];
		ec_calculate_skill_rating(v, server, prefix, sizeof(prefix));

		char line[128];
		snprintf(line, sizeof(line), "skill %s: %s", v->nickname.value, prefix);
		host->send_msg(server, v->peer, line);
	}
}
