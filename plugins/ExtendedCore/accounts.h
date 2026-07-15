#ifndef EC_ACCOUNTS_H
#define EC_ACCOUNTS_H

#include "peer.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define EC_ACCOUNTS_FILE "Player_Data/Accounts.json"

typedef struct ECAccount
{
	char     login[EC_MAX_LOGIN_LEN];
	uint8_t  op_level;
	uint32_t exp;
	uint8_t  level;
	uint32_t coins;
	uint32_t wins_surv;
	uint32_t wins_exe;
	char     language[8];
	int      inventory[EC_ITEM_COUNT];
} ECAccount;

bool ec_accounts_init(void);
void ec_accounts_shutdown(void);

bool ec_account_register(const char* login, const char* password);
bool ec_account_login(const char* login, const char* password, ECAccount* out);
bool ec_account_exists(const char* login);
bool ec_account_set_language(const char* login, const char* lang);
bool ec_account_set_progress(const char* login, uint32_t exp, uint8_t level, uint32_t coins);
bool ec_account_update_stats(const char* login, bool win_surv, bool win_exe);
bool ec_account_change_password(const char* login, const char* old_pass, const char* new_pass);
bool ec_account_add_item(const char* login, ECItemType item, int count);
bool ec_account_remove_item(const char* login, ECItemType item, int count);
bool ec_account_get_item_count(const char* login, ECItemType item, int* count);

#endif
