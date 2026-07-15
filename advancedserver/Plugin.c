#include <Plugin.h>
#include <Server.h>
#include <Packet.h>
#include <Lib.h>
#include <Log.h>
#include <string.h>
#include <stdlib.h>

#if PLUGIN_MAX != SERVER_PLUGIN_SLOTS
#error "PLUGIN_MAX must match SERVER_PLUGIN_SLOTS"
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <dirent.h>
#endif

typedef struct
{
	PluginInfo info;
	int        id;
	bool       active;
#ifdef _WIN32
	HMODULE    module;
#else
	void*      module;
#endif
} PluginSlot;

static PluginSlot  g_plugins[PLUGIN_MAX];
static int         g_plugin_count = 0;
static PluginHost  g_host;
static bool        g_ready = false;

#ifdef WITH_EXTENDED_CORE
bool extendedcore_plugin_entry(const PluginHost* host);
#endif

static bool host_send_msg(Server* server, ENetPeer* peer, const char* message)
{
	return server_send_msg(server, peer, message);
}

static bool host_broadcast_msg(Server* server, uint16_t sender, const char* message)
{
	return server_broadcast_msg(server, sender, message);
}

static bool host_send_packet(ENetPeer* peer, Packet* packet, bool reliable)
{
	return packet_send(peer, packet, reliable);
}

static bool host_broadcast_packet(Server* server, Packet* packet, bool reliable)
{
	return server_broadcast(server, packet, reliable);
}

static bool host_broadcast_packet_ex(Server* server, Packet* packet, bool reliable, uint16_t ignore)
{
	return server_broadcast_ex(server, packet, reliable, ignore);
}

static PeerData* host_find_peer(Server* server, uint16_t id)
{
	return server_find_peer(server, id);
}

static int host_peer_count(Server* server)
{
	return server_total(server);
}

static int host_ingame_count(Server* server)
{
	return server_ingame(server);
}

static void* host_peer_udata(PeerData* peer, int plugin_id)
{
	return plugin_peer_udata(peer, plugin_id);
}

static void* host_server_udata(Server* server, int plugin_id)
{
	return plugin_server_udata(server, plugin_id);
}

static bool host_disconnect(Server* server, ENetPeer* peer, int reason, const char* text)
{
	return server_disconnect(server, peer, (DisconnectReason)reason, text);
}

static Server* host_get_server(int index)
{
	return disaster_get(index);
}

static int host_server_count(void)
{
	return disaster_count();
}

static void host_init(void)
{
	memset(&g_host, 0, sizeof(g_host));
	g_host.api_version = PLUGIN_API_VERSION;
	g_host.send_msg = host_send_msg;
	g_host.broadcast_msg = host_broadcast_msg;
	g_host.send_packet = host_send_packet;
	g_host.broadcast_packet = host_broadcast_packet;
	g_host.broadcast_packet_ex = host_broadcast_packet_ex;
	g_host.find_peer = host_find_peer;
	g_host.peer_count = host_peer_count;
	g_host.ingame_count = host_ingame_count;
	g_host.peer_udata = host_peer_udata;
	g_host.server_udata = host_server_udata;
	g_host.disconnect = host_disconnect;
	g_host.get_server = host_get_server;
	g_host.server_count = host_server_count;
}

int plugin_register(const PluginInfo* info)
{
	if (!info || !info->name)
	{
		Warn("plugin_register: invalid info");
		return -1;
	}

	if (info->api_version != PLUGIN_API_VERSION)
	{
		Warn("plugin_register: %s api %d != host %d",
			info->name, info->api_version, PLUGIN_API_VERSION);
		return -1;
	}

	if (g_plugin_count >= PLUGIN_MAX)
	{
		Warn("plugin_register: plugin limit (%d) reached", PLUGIN_MAX);
		return -1;
	}

	for (int i = 0; i < g_plugin_count; i++)
	{
		if (strcmp(g_plugins[i].info.name, info->name) == 0)
		{
			Warn("plugin_register: duplicate name '%s'", info->name);
			return -1;
		}
	}

	PluginSlot* slot = &g_plugins[g_plugin_count];
	memset(slot, 0, sizeof(*slot));
	slot->info = *info;
	slot->id = g_plugin_count;
	slot->active = true;
	g_plugin_count++;

	Info("Plugin registered: " LOG_GRN "%s" LOG_RST " v%s (id %d)",
		info->name, info->version ? info->version : "?", slot->id);
	return slot->id;
}

void* plugin_peer_udata(PeerData* peer, int plugin_id)
{
	if (!peer || plugin_id < 0 || plugin_id >= g_plugin_count)
		return NULL;
	return peer->plugin_data[plugin_id];
}

void* plugin_server_udata(Server* server, int plugin_id)
{
	if (!server || plugin_id < 0 || plugin_id >= g_plugin_count)
		return NULL;
	return server->plugin_data[plugin_id];
}

static void alloc_server_slots(Server* server)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		size_t sz = g_plugins[i].info.server_data_size;
		if (sz == 0)
		{
			server->plugin_data[i] = NULL;
			continue;
		}
		server->plugin_data[i] = calloc(1, sz);
		if (!server->plugin_data[i])
			Warn("Failed to alloc server_data for plugin %s", g_plugins[i].info.name);
	}
}

static void free_server_slots(Server* server)
{
	for (int i = 0; i < PLUGIN_MAX; i++)
	{
		free(server->plugin_data[i]);
		server->plugin_data[i] = NULL;
	}
}

static void alloc_peer_slots(PeerData* peer)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		size_t sz = g_plugins[i].info.peer_data_size;
		if (sz == 0)
		{
			peer->plugin_data[i] = NULL;
			continue;
		}
		peer->plugin_data[i] = calloc(1, sz);
		if (!peer->plugin_data[i])
			Warn("Failed to alloc peer_data for plugin %s", g_plugins[i].info.name);
	}
}

static void free_peer_slots(PeerData* peer)
{
	for (int i = 0; i < PLUGIN_MAX; i++)
	{
		free(peer->plugin_data[i]);
		peer->plugin_data[i] = NULL;
	}
}

#ifdef PLUGIN_DYNAMIC_LOADING
static bool load_dynamic_plugin(const char* path)
{
#ifdef _WIN32
	HMODULE mod = LoadLibraryA(path);
	if (!mod)
	{
		Warn("Failed to load plugin: %s", path);
		return false;
	}
	PluginEntryFn entry = (PluginEntryFn)GetProcAddress(mod, "plugin_entry");
	if (!entry)
	{
		Warn("No plugin_entry in %s", path);
		FreeLibrary(mod);
		return false;
	}
	int before = g_plugin_count;
	if (!entry(&g_host))
	{
		Warn("plugin_entry failed: %s", path);
		FreeLibrary(mod);
		return false;
	}
	if (g_plugin_count > before)
		g_plugins[g_plugin_count - 1].module = mod;
	else
		FreeLibrary(mod);
#else
	void* mod = dlopen(path, RTLD_NOW);
	if (!mod)
	{
		Warn("Failed to load plugin: %s (%s)", path, dlerror());
		return false;
	}
	PluginEntryFn entry = (PluginEntryFn)dlsym(mod, "plugin_entry");
	if (!entry)
	{
		Warn("No plugin_entry in %s", path);
		dlclose(mod);
		return false;
	}
	int before = g_plugin_count;
	if (!entry(&g_host))
	{
		Warn("plugin_entry failed: %s", path);
		dlclose(mod);
		return false;
	}
	if (g_plugin_count > before)
		g_plugins[g_plugin_count - 1].module = mod;
	else
		dlclose(mod);
#endif
	return true;
}

static void load_plugins_dir(void)
{
#ifdef _WIN32
	WIN32_FIND_DATAA fd;
	HANDLE h = FindFirstFileA("plugins\\*.dll", &fd);
	if (h == INVALID_HANDLE_VALUE)
		return;
	do
	{
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		char path[MAX_PATH];
		snprintf(path, sizeof(path), "plugins\\%s", fd.cFileName);
		load_dynamic_plugin(path);
	} while (FindNextFileA(h, &fd));
	FindClose(h);
#else
	DIR* dir = opendir("plugins");
	if (!dir)
		return;
	struct dirent* ent;
	while ((ent = readdir(dir)) != NULL)
	{
		size_t len = strlen(ent->d_name);
		if (len < 4 || strcmp(ent->d_name + len - 3, ".so") != 0)
			continue;
		char path[512];
		snprintf(path, sizeof(path), "plugins/%s", ent->d_name);
		load_dynamic_plugin(path);
	}
	closedir(dir);
#endif
}
#endif

bool plugins_init(void)
{
	if (g_ready)
		return true;

	memset(g_plugins, 0, sizeof(g_plugins));
	g_plugin_count = 0;
	host_init();

#ifdef WITH_EXTENDED_CORE
	if (!extendedcore_plugin_entry(&g_host))
		Warn("ExtendedCore failed to register");
#endif

#ifdef PLUGIN_DYNAMIC_LOADING
	load_plugins_dir();
#endif

	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_init)
			g_plugins[i].info.hooks.on_init(&g_host);
	}

	g_ready = true;

	Info("--------- Plugins (%d) ---------", g_plugin_count);
	if (g_plugin_count == 0)
		Info("  (none)");
	else
	{
		for (int i = 0; i < g_plugin_count; i++)
		{
			const PluginInfo* info = &g_plugins[i].info;
			Info("  [%d] " LOG_GRN "%s" LOG_RST " v%s",
				i,
				info->name ? info->name : "?",
				info->version ? info->version : "?");
		}
	}
	Info("--------------------------------");
	return true;
}

void plugins_shutdown(void)
{
	if (!g_ready)
		return;

	for (int i = g_plugin_count - 1; i >= 0; i--)
	{
		if (g_plugins[i].info.hooks.on_shutdown)
			g_plugins[i].info.hooks.on_shutdown();
#ifdef _WIN32
		if (g_plugins[i].module)
			FreeLibrary(g_plugins[i].module);
#else
		if (g_plugins[i].module)
			dlclose(g_plugins[i].module);
#endif
		g_plugins[i].module = NULL;
		g_plugins[i].active = false;
	}

	g_plugin_count = 0;
	g_ready = false;
}

void plugins_server_created(Server* server)
{
	alloc_server_slots(server);
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_server_created)
			g_plugins[i].info.hooks.on_server_created(server);
	}
}

void plugins_server_destroyed(Server* server)
{
	for (int i = g_plugin_count - 1; i >= 0; i--)
	{
		if (g_plugins[i].info.hooks.on_server_destroyed)
			g_plugins[i].info.hooks.on_server_destroyed(server);
	}
	free_server_slots(server);
}

void plugins_peer_connect(PeerData* peer)
{
	alloc_peer_slots(peer);
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_peer_connect)
			g_plugins[i].info.hooks.on_peer_connect(peer);
	}
}

void plugins_peer_disconnect(PeerData* peer)
{
	for (int i = g_plugin_count - 1; i >= 0; i--)
	{
		if (g_plugins[i].info.hooks.on_peer_disconnect)
			g_plugins[i].info.hooks.on_peer_disconnect(peer);
	}
	free_peer_slots(peer);
}

PluginResult plugins_peer_identity(PeerData* peer)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		PluginHooks* h = &g_plugins[i].info.hooks;
		if (!h->on_peer_identity)
			continue;
		PluginResult r = h->on_peer_identity(peer);
		if (r == PLUGIN_HANDLED)
			return PLUGIN_HANDLED;
	}
	return PLUGIN_CONTINUE;
}

void plugins_peer_identity_ok(PeerData* peer)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_peer_identity_ok)
			g_plugins[i].info.hooks.on_peer_identity_ok(peer);
	}
}

void plugins_peer_join(PeerData* peer)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_peer_join)
			g_plugins[i].info.hooks.on_peer_join(peer);
	}
}

void plugins_peer_leave(PeerData* peer)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_peer_leave)
			g_plugins[i].info.hooks.on_peer_leave(peer);
	}
}

PluginResult plugins_packet(PeerData* peer, Packet* packet)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		PluginHooks* h = &g_plugins[i].info.hooks;
		if (!h->on_packet)
			continue;
		PluginResult r = h->on_packet(peer, packet);
		if (r == PLUGIN_HANDLED)
			return PLUGIN_HANDLED;
	}
	return PLUGIN_CONTINUE;
}

PluginResult plugins_chat(PeerData* peer, String* msg)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		PluginHooks* h = &g_plugins[i].info.hooks;
		if (!h->on_chat)
			continue;
		PluginResult r = h->on_chat(peer, msg);
		if (r == PLUGIN_HANDLED)
			return PLUGIN_HANDLED;
	}
	return PLUGIN_CONTINUE;
}

PluginResult plugins_command(PeerData* peer, unsigned long hash, String* msg)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		PluginHooks* h = &g_plugins[i].info.hooks;
		if (!h->on_command)
			continue;
		PluginResult r = h->on_command(peer, hash, msg);
		if (r == PLUGIN_HANDLED)
			return PLUGIN_HANDLED;
	}
	return PLUGIN_CONTINUE;
}

void plugins_tick(Server* server, double delta)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_tick)
			g_plugins[i].info.hooks.on_tick(server, delta);
	}
}

void plugins_state_change(Server* server, int old_state, int new_state)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_state_change)
			g_plugins[i].info.hooks.on_state_change(server, old_state, new_state);
	}
}

void plugins_game_start(Server* server)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_game_start)
			g_plugins[i].info.hooks.on_game_start(server);
	}
}

void plugins_game_end(Server* server, int ending)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_game_end)
			g_plugins[i].info.hooks.on_game_end(server, ending);
	}
}

void plugins_results(Server* server)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_results)
			g_plugins[i].info.hooks.on_results(server);
	}
}

void plugins_player_data(PeerData* peer)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_player_data)
			g_plugins[i].info.hooks.on_player_data(peer);
	}
}

void plugins_game_player_tick(Server* server, PeerData* peer)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		if (g_plugins[i].info.hooks.on_game_player_tick)
			g_plugins[i].info.hooks.on_game_player_tick(server, peer);
	}
}

PluginResult plugins_player_death(PeerData* peer)
{
	for (int i = 0; i < g_plugin_count; i++)
	{
		PluginHooks* h = &g_plugins[i].info.hooks;
		if (!h->on_player_death)
			continue;
		PluginResult r = h->on_player_death(peer);
		if (r == PLUGIN_HANDLED)
			return PLUGIN_HANDLED;
	}
	return PLUGIN_CONTINUE;
}
