#include "accounts.h"
#include "sha256.h"
#include <cJSON.h>
#include <Log.h>
#include <io/Threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#define ec_mkdir(path) _mkdir(path)
#else
#include <sys/stat.h>
#define ec_mkdir(path) mkdir(path, 0777)
#endif

static cJSON* g_accounts = NULL;
static Mutex  g_accounts_mutex;
static bool   g_accounts_ready = false;

static void bytes_to_hex(const uint8_t* bytes, size_t len, char* hex)
{
	for (size_t i = 0; i < len; i++)
		sprintf(hex + i * 2, "%02x", bytes[i]);
	hex[len * 2] = '\0';
}

static void hex_to_bytes(const char* hex, uint8_t* bytes, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		unsigned int val = 0;
		sscanf(hex + i * 2, "%02x", &val);
		bytes[i] = (uint8_t)val;
	}
}

static bool account_save(void)
{
	char* json_str = cJSON_Print(g_accounts);
	if (!json_str)
		return false;
	FILE* f = fopen(EC_ACCOUNTS_FILE, "w");
	if (!f)
	{
		free(json_str);
		return false;
	}
	fputs(json_str, f);
	fclose(f);
	free(json_str);
	return true;
}

bool ec_accounts_init(void)
{
	if (g_accounts_ready)
		return true;

	ec_mkdir("Player_Data");
	MutexCreate(g_accounts_mutex);

	FILE* f = fopen(EC_ACCOUNTS_FILE, "r");
	if (!f)
	{
		g_accounts = cJSON_CreateObject();
		g_accounts_ready = account_save();
		return g_accounts_ready;
	}

	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	rewind(f);
	char* data = malloc((size_t)sz + 1);
	if (!data)
	{
		fclose(f);
		return false;
	}
	fread(data, 1, (size_t)sz, f);
	data[sz] = '\0';
	fclose(f);

	g_accounts = cJSON_Parse(data);
	free(data);
	g_accounts_ready = (g_accounts != NULL);
	if (!g_accounts_ready)
		g_accounts = cJSON_CreateObject();
	return true;
}

void ec_accounts_shutdown(void)
{
	if (!g_accounts_ready)
		return;
	/* Shutdown is single-threaded after workers stop. */
	account_save();
	cJSON_Delete(g_accounts);
	g_accounts = NULL;
	g_accounts_ready = false;
}

bool ec_account_register(const char* login, const char* password)
{
	if (!login || !password || strlen(login) >= EC_MAX_LOGIN_LEN || strlen(password) < 3)
		return false;

	MutexLock(g_accounts_mutex);
	if (cJSON_HasObjectItem(g_accounts, login))
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}

	uint8_t salt[EC_SALT_LEN];
	for (int i = 0; i < EC_SALT_LEN; i++)
		salt[i] = (uint8_t)(rand() % 256);

	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (const uint8_t*)password, strlen(password));
	sha256_update(&ctx, salt, EC_SALT_LEN);
	uint8_t hash[SHA256_BLOCK_SIZE];
	sha256_final(&ctx, hash);

	cJSON* acc = cJSON_CreateObject();
	cJSON_AddNumberToObject(acc, "op_level", 0);
	cJSON_AddNumberToObject(acc, "exp", 0);
	cJSON_AddNumberToObject(acc, "level", 0);
	cJSON_AddNumberToObject(acc, "coins", 0);
	cJSON_AddNumberToObject(acc, "wins_surv", 0);
	cJSON_AddNumberToObject(acc, "wins_exe", 0);
	cJSON_AddStringToObject(acc, "language", "en");

	cJSON* inv = cJSON_CreateObject();
	cJSON_AddNumberToObject(inv, "demon_start", 0);
	cJSON_AddNumberToObject(inv, "red_rings", 0);
	cJSON_AddNumberToObject(inv, "shield", 0);
	cJSON_AddNumberToObject(inv, "map_select", 0);
	cJSON_AddItemToObject(acc, "inventory", inv);

	char salt_hex[EC_SALT_LEN * 2 + 1];
	char hash_hex[SHA256_BLOCK_SIZE * 2 + 1];
	bytes_to_hex(salt, EC_SALT_LEN, salt_hex);
	bytes_to_hex(hash, SHA256_BLOCK_SIZE, hash_hex);
	cJSON_AddStringToObject(acc, "salt", salt_hex);
	cJSON_AddStringToObject(acc, "hash", hash_hex);

	cJSON_AddItemToObject(g_accounts, login, acc);
	bool ok = account_save();
	MutexUnlock(g_accounts_mutex);
	return ok;
}

bool ec_account_login(const char* login, const char* password, ECAccount* out)
{
	if (!out || !login || !password)
		return false;
	memset(out, 0, sizeof(*out));

	MutexLock(g_accounts_mutex);
	cJSON* acc_json = cJSON_GetObjectItem(g_accounts, login);
	if (!acc_json)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}

	const char* salt_hex = cJSON_GetStringValue(cJSON_GetObjectItem(acc_json, "salt"));
	const char* hash_hex = cJSON_GetStringValue(cJSON_GetObjectItem(acc_json, "hash"));
	if (!salt_hex || !hash_hex)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}

	uint8_t salt[EC_SALT_LEN], expected_hash[SHA256_BLOCK_SIZE];
	hex_to_bytes(salt_hex, salt, EC_SALT_LEN);
	hex_to_bytes(hash_hex, expected_hash, SHA256_BLOCK_SIZE);

	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (const uint8_t*)password, strlen(password));
	sha256_update(&ctx, salt, EC_SALT_LEN);
	uint8_t computed[SHA256_BLOCK_SIZE];
	sha256_final(&ctx, computed);

	if (memcmp(computed, expected_hash, SHA256_BLOCK_SIZE) != 0)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}

	strncpy(out->login, login, EC_MAX_LOGIN_LEN - 1);
	out->op_level = (uint8_t)cJSON_GetNumberValue(cJSON_GetObjectItem(acc_json, "op_level"));
	out->exp = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(acc_json, "exp"));
	out->level = (uint8_t)cJSON_GetNumberValue(cJSON_GetObjectItem(acc_json, "level"));
	out->coins = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(acc_json, "coins"));
	out->wins_surv = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(acc_json, "wins_surv"));
	out->wins_exe = (uint32_t)cJSON_GetNumberValue(cJSON_GetObjectItem(acc_json, "wins_exe"));

	const char* lang = cJSON_GetStringValue(cJSON_GetObjectItem(acc_json, "language"));
	if (lang && lang[0])
		strncpy(out->language, lang, sizeof(out->language) - 1);
	else
		strcpy(out->language, "en");

	cJSON* inv = cJSON_GetObjectItem(acc_json, "inventory");
	if (inv)
	{
		out->inventory[EC_ITEM_DEMON_START] = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(inv, "demon_start"));
		out->inventory[EC_ITEM_RED_RINGS] = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(inv, "red_rings"));
		out->inventory[EC_ITEM_SHIELD] = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(inv, "shield"));
		out->inventory[EC_ITEM_MAP_SELECT] = (int)cJSON_GetNumberValue(cJSON_GetObjectItem(inv, "map_select"));
	}

	MutexUnlock(g_accounts_mutex);
	return true;
}

bool ec_account_exists(const char* login)
{
	if (!login)
		return false;
	MutexLock(g_accounts_mutex);
	bool exists = cJSON_HasObjectItem(g_accounts, login);
	MutexUnlock(g_accounts_mutex);
	return exists;
}

bool ec_account_set_language(const char* login, const char* lang)
{
	MutexLock(g_accounts_mutex);
	cJSON* acc = cJSON_GetObjectItem(g_accounts, login);
	if (!acc)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}
	cJSON_ReplaceItemInObject(acc, "language", cJSON_CreateString(lang));
	bool ok = account_save();
	MutexUnlock(g_accounts_mutex);
	return ok;
}

bool ec_account_set_progress(const char* login, uint32_t exp, uint8_t level, uint32_t coins)
{
	MutexLock(g_accounts_mutex);
	cJSON* acc = cJSON_GetObjectItem(g_accounts, login);
	if (!acc)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}

	cJSON* jexp = cJSON_GetObjectItem(acc, "exp");
	cJSON* jlevel = cJSON_GetObjectItem(acc, "level");
	cJSON* jcoins = cJSON_GetObjectItem(acc, "coins");
	if (!jexp) cJSON_AddNumberToObject(acc, "exp", exp);
	else cJSON_SetNumberValue(jexp, exp);
	if (!jlevel) cJSON_AddNumberToObject(acc, "level", level);
	else cJSON_SetNumberValue(jlevel, level);
	if (!jcoins) cJSON_AddNumberToObject(acc, "coins", coins);
	else cJSON_SetNumberValue(jcoins, coins);

	bool ok = account_save();
	MutexUnlock(g_accounts_mutex);
	return ok;
}

bool ec_account_update_stats(const char* login, bool win_surv, bool win_exe)
{
	MutexLock(g_accounts_mutex);
	cJSON* acc = cJSON_GetObjectItem(g_accounts, login);
	if (!acc)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}
	if (win_surv)
	{
		cJSON* ws = cJSON_GetObjectItem(acc, "wins_surv");
		if (ws) cJSON_SetNumberValue(ws, ws->valuedouble + 1);
	}
	if (win_exe)
	{
		cJSON* we = cJSON_GetObjectItem(acc, "wins_exe");
		if (we) cJSON_SetNumberValue(we, we->valuedouble + 1);
	}
	bool ok = account_save();
	MutexUnlock(g_accounts_mutex);
	return ok;
}

bool ec_account_change_password(const char* login, const char* old_pass, const char* new_pass)
{
	if (!new_pass || strlen(new_pass) < 3)
		return false;

	MutexLock(g_accounts_mutex);
	cJSON* acc_json = cJSON_GetObjectItem(g_accounts, login);
	if (!acc_json)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}

	const char* salt_hex = cJSON_GetStringValue(cJSON_GetObjectItem(acc_json, "salt"));
	const char* hash_hex = cJSON_GetStringValue(cJSON_GetObjectItem(acc_json, "hash"));
	if (!salt_hex || !hash_hex)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}

	uint8_t salt[EC_SALT_LEN], expected[SHA256_BLOCK_SIZE];
	hex_to_bytes(salt_hex, salt, EC_SALT_LEN);
	hex_to_bytes(hash_hex, expected, SHA256_BLOCK_SIZE);

	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, (const uint8_t*)old_pass, strlen(old_pass));
	sha256_update(&ctx, salt, EC_SALT_LEN);
	uint8_t computed[SHA256_BLOCK_SIZE];
	sha256_final(&ctx, computed);
	if (memcmp(computed, expected, SHA256_BLOCK_SIZE) != 0)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}

	for (int i = 0; i < EC_SALT_LEN; i++)
		salt[i] = (uint8_t)(rand() % 256);
	sha256_init(&ctx);
	sha256_update(&ctx, (const uint8_t*)new_pass, strlen(new_pass));
	sha256_update(&ctx, salt, EC_SALT_LEN);
	sha256_final(&ctx, computed);

	char salt_out[EC_SALT_LEN * 2 + 1];
	char hash_out[SHA256_BLOCK_SIZE * 2 + 1];
	bytes_to_hex(salt, EC_SALT_LEN, salt_out);
	bytes_to_hex(computed, SHA256_BLOCK_SIZE, hash_out);
	cJSON_ReplaceItemInObject(acc_json, "salt", cJSON_CreateString(salt_out));
	cJSON_ReplaceItemInObject(acc_json, "hash", cJSON_CreateString(hash_out));

	bool ok = account_save();
	MutexUnlock(g_accounts_mutex);
	return ok;
}

static const char* item_key(ECItemType item)
{
	static const char* keys[] = { NULL, "demon_start", "red_rings", "shield", "map_select" };
	if (item <= EC_ITEM_NONE || item >= EC_ITEM_COUNT)
		return NULL;
	return keys[item];
}

bool ec_account_add_item(const char* login, ECItemType item, int count)
{
	const char* key = item_key(item);
	if (!key)
		return false;

	MutexLock(g_accounts_mutex);
	cJSON* acc = cJSON_GetObjectItem(g_accounts, login);
	if (!acc)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}
	cJSON* inv = cJSON_GetObjectItem(acc, "inventory");
	if (!inv)
	{
		inv = cJSON_CreateObject();
		cJSON_AddItemToObject(acc, "inventory", inv);
	}
	cJSON* cur = cJSON_GetObjectItem(inv, key);
	if (!cur)
		cJSON_AddNumberToObject(inv, key, count);
	else
		cJSON_SetNumberValue(cur, cur->valuedouble + count);
	bool ok = account_save();
	MutexUnlock(g_accounts_mutex);
	return ok;
}

bool ec_account_remove_item(const char* login, ECItemType item, int count)
{
	const char* key = item_key(item);
	if (!key)
		return false;

	MutexLock(g_accounts_mutex);
	cJSON* acc = cJSON_GetObjectItem(g_accounts, login);
	if (!acc)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}
	cJSON* inv = cJSON_GetObjectItem(acc, "inventory");
	if (!inv)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}
	cJSON* cur = cJSON_GetObjectItem(inv, key);
	if (!cur || cur->valuedouble < count)
	{
		MutexUnlock(g_accounts_mutex);
		return false;
	}
	cJSON_SetNumberValue(cur, cur->valuedouble - count);
	bool ok = account_save();
	MutexUnlock(g_accounts_mutex);
	return ok;
}

bool ec_account_get_item_count(const char* login, ECItemType item, int* count)
{
	const char* key = item_key(item);
	if (!key || !count)
		return false;
	*count = 0;

	MutexLock(g_accounts_mutex);
	cJSON* acc = cJSON_GetObjectItem(g_accounts, login);
	if (!acc)
	{
		MutexUnlock(g_accounts_mutex);
		return true;
	}
	cJSON* inv = cJSON_GetObjectItem(acc, "inventory");
	if (!inv)
	{
		MutexUnlock(g_accounts_mutex);
		return true;
	}
	cJSON* cur = cJSON_GetObjectItem(inv, key);
	*count = cur ? (int)cur->valuedouble : 0;
	MutexUnlock(g_accounts_mutex);
	return true;
}
