#include "chat.h"
#include "accounts.h"
#include "locale.h"
#include "peer.h"
#include "menu.h"
#include <Lib.h>
#include <Server.h>
#include <Packet.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static const uint32_t EXP_TABLE[21] = {
	0, 0, 20, 45, 75, 110, 150, 195, 245, 300,
	360, 425, 495, 570, 650, 735, 825, 920, 1020, 1125, 1235
};

/* Match original ExtendedCore thresholds more closely where possible */
static const uint32_t EXP_THRESH[21] = {
	0,
	10, 30, 60, 100, 150, 210, 280, 360, 450, 550,
	660, 780, 910, 1050, 1200, 1360, 1530, 1710, 1900, 2100
};

static ECPeer* peer_ext(PeerData* peer, int plugin_id)
{
	return (ECPeer*)plugin_peer_udata(peer, plugin_id);
}

static const char* peer_lang(ECPeer* ec)
{
	if (ec && ec->language[0])
		return ec->language;
	return "en";
}

static bool cmd_prefix(const char* msg, const char* cmd)
{
	size_t n = strlen(cmd);
	if (strncmp(msg, cmd, n) != 0)
		return false;
	return msg[n] == '\0' || isspace((unsigned char)msg[n]);
}

static void sync_level(ECPeer* ec, bool* leveled)
{
	uint8_t prev = ec->level;
	while (ec->level < 20 && ec->exp >= EXP_THRESH[ec->level + 1])
		ec->level++;
	if (ec->level > prev)
	{
		strncpy(ec->title, ec_title_for_level(ec->level, peer_lang(ec)), sizeof(ec->title) - 1);
		ec->title[sizeof(ec->title) - 1] = '\0';
		if (leveled)
			*leveled = true;
	}
	ec_account_set_progress(ec->login, ec->exp, ec->level, ec->coins);
	(void)EXP_TABLE;
}

static void apply_account(PeerData* peer, ECPeer* ec, const ECAccount* acc)
{
	ec->authenticated = true;
	strncpy(ec->login, acc->login, EC_MAX_LOGIN_LEN - 1);
	ec->auth_op_level = acc->op_level;
	peer->op = acc->op_level;
	ec->login_timeout = 0;
	ec->login_warn_timer = 0;
	ec->level = acc->level;
	ec->exp = acc->exp;
	ec->coins = acc->coins;
	ec->wins_surv = acc->wins_surv;
	ec->wins_exe = acc->wins_exe;
	memcpy(ec->inventory, acc->inventory, sizeof(ec->inventory));
	strncpy(ec->language, acc->language, sizeof(ec->language) - 1);
	if (ec->language[0] == '\0')
		strcpy(ec->language, "en");
	strncpy(ec->title, ec_title_for_level(ec->level, peer_lang(ec)), sizeof(ec->title) - 1);
	sync_level(ec, NULL);
}

static PluginResult handle_auth_cmds(PeerData* peer, ECPeer* ec, String* msg, const PluginHost* host)
{
	const char* text = msg->value;
	char lang_buf[8];
	const char* auth_lang = ec->language[0] ? ec->language : "en";
	(void)lang_buf;

	if (cmd_prefix(text, ".register"))
	{
		char pwd1[64], pwd2[64];
		if (sscanf(text, "%*[^ ] %63s %63s", pwd1, pwd2) == 2)
		{
			if (strcmp(pwd1, pwd2) != 0)
				host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_CHPASS_MISMATCH, auth_lang));
			else if (ec_account_exists(peer->nickname.value))
				host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_REGISTER_FAIL_EXISTS, auth_lang));
			else if (ec_account_register(peer->nickname.value, pwd1))
			{
				if (ec->language[0])
					ec_account_set_language(peer->nickname.value, ec->language);
				host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_REGISTER_SUCCESS, peer_lang(ec)));
				host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_HINT_LANG, peer_lang(ec)));
			}
			else
				host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_REGISTER_FAIL_SHORT, auth_lang));
		}
		else
			host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_REGISTER_USAGE, auth_lang));
		return PLUGIN_HANDLED;
	}

	if (cmd_prefix(text, ".login"))
	{
		char pwd[64];
		if (sscanf(text, "%*[^ ] %63s", pwd) == 1)
		{
			ECAccount acc;
			if (ec_account_login(peer->nickname.value, pwd, &acc))
			{
				apply_account(peer, ec, &acc);
				char login_msg[128];
				snprintf(login_msg, sizeof(login_msg), ec_msg(EC_MSG_LOGIN_SUCCESS, peer_lang(ec)), peer->op);
				host->send_msg(peer->server, peer->peer, login_msg);
				host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_HINT_MENU, peer_lang(ec)));
			}
			else
				host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_LOGIN_FAIL, auth_lang));
		}
		else
			host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_LOGIN_USAGE, auth_lang));
		return PLUGIN_HANDLED;
	}

	if (cmd_prefix(text, ".registration"))
	{
		if (ec_account_exists(peer->nickname.value))
			host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_REG_STATUS_YES, auth_lang));
		else
			host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_REG_STATUS_NO, auth_lang));
		return PLUGIN_HANDLED;
	}

	return PLUGIN_CONTINUE;
}

PluginResult ec_chat_handle(PeerData* peer, String* msg, const PluginHost* host, int plugin_id)
{
	ECPeer* ec = peer_ext(peer, plugin_id);
	if (!ec || !host)
		return PLUGIN_CONTINUE;

	PluginResult auth = handle_auth_cmds(peer, ec, msg, host);
	if (auth == PLUGIN_HANDLED)
		return PLUGIN_HANDLED;

	if (ec_menu_handle_chat(peer, msg, host, plugin_id))
		return PLUGIN_HANDLED;

	/* Mute normal chat until registered accounts log in */
	if (!ec->authenticated && ec_account_exists(peer->nickname.value))
		return PLUGIN_HANDLED;

	/* Format (lvl title) and skip peers with open menus */
	if (ec->authenticated && msg->value[0] != '.' && msg->value[0] != ':')
	{
		char formatted[160];
		snprintf(formatted, sizeof(formatted), "(lvl %d %s) %s",
			ec->level,
			ec->title[0] ? ec->title : ec_title_for_level(ec->level, peer_lang(ec)),
			msg->value);

		Packet pack;
		PacketCreate(&pack, CLIENT_CHAT_MESSAGE);
		PacketWrite(&pack, packet_write16, peer->id);
		PacketWrite(&pack, packet_writestr, string_lower(__Str(formatted)));

		for (size_t i = 0; i < peer->server->peers.capacity; i++)
		{
			PeerData* other = (PeerData*)peer->server->peers.ptr[i];
			if (!other)
				continue;
			if (ec_menu_is_muted(other, plugin_id))
				continue;
			host->send_packet(other->peer, &pack, true);
		}
		return PLUGIN_HANDLED;
	}

	return PLUGIN_CONTINUE;
}

void ec_auth_send_welcome(PeerData* peer, const PluginHost* host, int plugin_id)
{
	ECPeer* ec = peer_ext(peer, plugin_id);
	if (!ec || !host || ec->welcome_sent)
		return;

	if (!ec->language[0])
		strcpy(ec->language, "en");

	ec->welcome_sent = true;

	if (ec->authenticated)
	{
		host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_HINT_MENU, peer_lang(ec)));
		return;
	}

	if (!ec_account_exists(peer->nickname.value))
	{
		host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_REG_STATUS_NO, peer_lang(ec)));
		host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_REGISTER_USAGE, peer_lang(ec)));
		return;
	}

	if (ec->login_timeout <= 0 && ec->login_warn_timer <= 0)
	{
		/* Countdown starts on first AUTH_WARNING, not on join. */
		ec->login_timeout = 0;
		ec->login_warn_timer = 4.0 * TICKSPERSEC;
	}
	host->send_msg(peer->server, peer->peer, ec_msg(EC_MSG_LOGIN_USAGE, peer_lang(ec)));
}

void ec_auth_on_identity_ok(PeerData* peer, const PluginHost* host, int plugin_id)
{
	/* Prefer lobby players-request for in-lobby joins; still prompt spectators/queue. */
	if (peer && peer->in_game && peer->server && peer->server->state == ST_LOBBY)
		return;
	ec_auth_send_welcome(peer, host, plugin_id);
}

void ec_auth_tick(Server* server, double delta, const PluginHost* host, int plugin_id)
{
	if (!server || !host)
		return;

	for (size_t i = 0; i < server->peers.capacity; i++)
	{
		PeerData* p = (PeerData*)server->peers.ptr[i];
		if (!p)
			continue;
		ECPeer* ec = peer_ext(p, plugin_id);
		if (!ec)
			continue;

		/* After stock lobby welcome (help / location / MOTD) in the same frame. */
		if (ec->welcome_pending) {
			ec->welcome_pending = false;
			ec_auth_send_welcome(p, host, plugin_id);
		}

		/* Auth wait: warn_timer until first warning, then 10s kick countdown. */
		if (ec->authenticated || (ec->login_timeout <= 0 && ec->login_warn_timer <= 0))
			continue;

		ec->login_warn_timer -= delta;
		if (ec->login_warn_timer <= 0)
		{
			host->send_msg(server, p->peer, ec_msg(EC_MSG_AUTH_WARNING, peer_lang(ec)));
			if (ec->login_timeout <= 0)
				ec->login_timeout = 10.0 * TICKSPERSEC;
			ec->login_warn_timer = 4.0 * TICKSPERSEC;
		}

		if (ec->login_timeout > 0)
		{
			ec->login_timeout -= delta;
			if (ec->login_timeout <= 0)
			{
				ec->login_warn_timer = 0;
				host->disconnect(server, p->peer, DR_KICKEDBYHOST,
					ec_msg(EC_MSG_AUTH_KICK, peer_lang(ec)));
			}
		}

		if (ec->pending_level_up && ec->level_up_notify_timer > 0)
		{
			ec->level_up_notify_timer -= delta;
			if (ec->level_up_notify_timer <= 0)
			{
				char buf[128];
				snprintf(buf, sizeof(buf), "level up: %d (%s)",
					ec->pending_level_up,
					ec_title_for_level(ec->pending_level_up, peer_lang(ec)));
				host->send_msg(server, p->peer, buf);
				ec->pending_level_up = 0;
			}
		}
	}
}

void ec_rewards_on_game_end(Server* server, int ending, int plugin_id)
{
	if (!server)
		return;

	int plrs = server_ingame(server);
	if (plrs < 2)
		plrs = 2;

	for (size_t i = 0; i < server->peers.capacity; i++)
	{
		PeerData* v = (PeerData*)server->peers.ptr[i];
		if (!v || !v->in_game)
			continue;
		ECPeer* ec = peer_ext(v, plugin_id);
		if (!ec || !ec->authenticated || !ec->login[0])
			continue;

		bool is_exe = (v->id == server->game.exe);
		bool win_surv = ending == ED_SURVWIN && !is_exe
			&& !(v->plr.flags & (PLAYER_DEAD | PLAYER_DEMONIZED));
		bool win_exe = ending == ED_EXEWIN && is_exe;

		int exp_gain = 5;
		if (win_surv) exp_gain += 25;
		if (win_exe) exp_gain += 30;

		int coins_gain = 0;
		if (win_surv)
			coins_gain = plrs >= 3 ? 5 : 2;
		else if (win_exe)
			coins_gain = plrs >= 3 ? 10 : 2;

		ec->exp += (uint32_t)exp_gain;
		ec->coins += (uint32_t)coins_gain;
		if (win_surv) ec->wins_surv++;
		if (win_exe) ec->wins_exe++;

		bool leveled = false;
		sync_level(ec, &leveled);
		if (leveled)
		{
			ec->pending_level_up = ec->level;
			ec->level_up_notify_timer = 3.0 * TICKSPERSEC;
		}

		if (win_surv)
			ec_account_update_stats(ec->login, true, false);
		else if (win_exe)
			ec_account_update_stats(ec->login, false, true);
	}
}
