#include "locale.h"
#include <Colors.h>
#include <stdint.h>
#include <string.h>

static const char* g_ru[EC_MSG_COUNT];
static const char* g_en[EC_MSG_COUNT];
static const char* g_es[EC_MSG_COUNT];

static const char* titles_ru[] = {
	"свежее мясо", "новобранец", "беглец", "осторожный", "хитрец",
	"боец", "ветеран", "мрачный", "беспощадный", "охотник",
	"призрак", "каратель", "безумец", "кровавый", "палач",
	"темный лорд", "сеятель хаоса", "легенда", "несокрушимый", "полубог", "бог"
};

static const char* titles_en[] = {
	"fresh meat", "recruit", "fugitive", "wary", "trickster",
	"fighter", "veteran", "grim", "ruthless", "hunter",
	"ghost", "punisher", "madman", "bloody", "executioner",
	"dark lord", "chaos maker", "legend", "unbreakable", "demigod", "god"
};

static const char* titles_es[] = {
	"carne fresca", "recluta", "fugitivo", "cauteloso", "astuto",
	"luchador", "veterano", "sombrio", "despiadado", "cazador",
	"fantasma", "castigador", "loco", "sanguinario", "verdugo",
	"senor oscuro", "caos", "leyenda", "indestructible", "semidios", "dios"
};

void ec_locale_init(void)
{
	g_ru[EC_MSG_REGISTER_SUCCESS] = "регистрация успешна. используйте .login (пароль) для входа.";
	g_ru[EC_MSG_REGISTER_FAIL_EXISTS] = "этот ник уже зарегистрирован.";
	g_ru[EC_MSG_REGISTER_FAIL_SHORT] = "пароль слишком короткий (минимум 3 символа).";
	g_ru[EC_MSG_LOGIN_SUCCESS] = "вход выполнен. ваш уровень прав: %d.";
	g_ru[EC_MSG_LOGIN_FAIL] = "неверный пароль или ник не зарегистрирован.";
	g_ru[EC_MSG_LOGIN_USAGE] = CLRCODE_YLW ".login (пароль)" CLRCODE_RST;
	g_ru[EC_MSG_REGISTER_USAGE] = CLRCODE_YLW ".register (пароль) (пароль)" CLRCODE_RST;
	g_ru[EC_MSG_REG_STATUS_YES] = CLRCODE_YLW "аккаунт зарегистрирован. используйте .login (пароль)" CLRCODE_RST;
	g_ru[EC_MSG_REG_STATUS_NO] = CLRCODE_YLW "аккаунт не зарегистрирован. пожалуйста зарегистрируйтесь:" CLRCODE_RST;
	g_ru[EC_MSG_AUTH_WARNING] = "аккаунт защищён паролем. введите .login (пароль) в течение 10 секунд.";
	g_ru[EC_MSG_AUTH_KICK] = "вы не вошли вовремя.";
	g_ru[EC_MSG_HINT_MENU] = "откройте меню: .menu / .m";
	g_ru[EC_MSG_HINT_LANG] = "язык: .lang";
	g_ru[EC_MSG_NEED_AUTH] = "только для зарегистрированных. .login / .register";
	g_ru[EC_MSG_MENU_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "меню" CLRCODE_PUR " --------" CLRCODE_RST;
	g_ru[EC_MSG_LEVEL_INFO] = "уровень: %d (%s)\nопыт: %d\nмонет: %d\nпобед surv/exe: %d/%d";
	g_ru[EC_MSG_CHPASS_MISMATCH] = "пароли не совпадают.";
	g_ru[EC_MSG_SKILL_LINE] = "skill: %s";
	g_ru[EC_MSG_MENU_OPTION_LEVEL] = "уровень и статистика"; g_ru[EC_MSG_MENU_OPTION_SHOP] = "магазин"; g_ru[EC_MSG_MENU_OPTION_INVENTORY] = "инвентарь"; g_ru[EC_MSG_MENU_OPTION_LANGUAGE] = "язык"; g_ru[EC_MSG_MENU_OPTION_HELP] = "помощь"; g_ru[EC_MSG_MENU_OPTION_CHPASS] = "сменить пароль";
	g_ru[EC_MSG_LANG_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "язык" CLRCODE_PUR " --------" CLRCODE_RST; g_ru[EC_MSG_LANG_CHANGED] = "язык изменён.";
	g_ru[EC_MSG_SHOP_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "магазин" CLRCODE_PUR " --------" CLRCODE_RST; g_ru[EC_MSG_SHOP_DEMON] = "демон на старте (100)"; g_ru[EC_MSG_SHOP_RINGS] = "красные кольца (30)"; g_ru[EC_MSG_SHOP_SHIELD] = "тотем бессмертия (50)"; g_ru[EC_MSG_SHOP_LOTTERY] = "лотерея (10)"; g_ru[EC_MSG_SHOP_MAP] = "выбор карты (20)";
	g_ru[EC_MSG_SHOP_NO_COINS] = "недостаточно монет."; g_ru[EC_MSG_SHOP_LIMIT] = "лимит предметов: %d."; g_ru[EC_MSG_LOTTERY_DEMON] = "лотерея: демон на старте!"; g_ru[EC_MSG_LOTTERY_RINGS] = "лотерея: красные кольца!"; g_ru[EC_MSG_LOTTERY_SHIELD] = "лотерея: тотем!"; g_ru[EC_MSG_LOTTERY_REFUND] = "лотерея: возврат монет."; g_ru[EC_MSG_LOTTERY_LOSE] = "лотерея: не повезло.";
	g_ru[EC_MSG_INVENTORY_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "инвентарь" CLRCODE_PUR " --------" CLRCODE_RST; g_ru[EC_MSG_INVENTORY_EMPTY] = "инвентарь пуст."; g_ru[EC_MSG_INV_DEMON] = "демон на старте x%d"; g_ru[EC_MSG_INV_RINGS] = "красные кольца x%d"; g_ru[EC_MSG_INV_SHIELD] = "тотем бессмертия x%d"; g_ru[EC_MSG_INV_MAP] = "выбор карты x%d";
	g_ru[EC_MSG_NO_ITEM] = "нет предмета."; g_ru[EC_MSG_DEMON_CONDITIONS] = "нужно минимум 4 игрока."; g_ru[EC_MSG_DEMON_USED] = "демон на старте уже использован."; g_ru[EC_MSG_DEMON_OK] = "демон на старте активирован."; g_ru[EC_MSG_RINGS_OK] = "красные кольца активированы."; g_ru[EC_MSG_SHIELD_OK] = "тотем активирован.";
	g_ru[EC_MSG_MAPS_HEADER] = "выберите карту:"; g_ru[EC_MSG_MAPS_LINE] = "%d. %s"; g_ru[EC_MSG_MAP_BAD_NUM] = "неверный номер карты."; g_ru[EC_MSG_MAP_READY] = "нельзя выбрать карту во время отсчёта."; g_ru[EC_MSG_MAP_WAIT] = "%s выбрал карту %s.";
	g_ru[EC_MSG_CHPASS_OLD] = "введите старый пароль (0 отмена):"; g_ru[EC_MSG_CHPASS_NEW] = "введите новый пароль:"; g_ru[EC_MSG_CHPASS_CONFIRM] = "подтвердите новый пароль:"; g_ru[EC_MSG_CHPASS_SUCCESS] = "пароль изменён."; g_ru[EC_MSG_CHPASS_FAIL] = "не удалось сменить пароль.";
	g_ru[EC_MSG_HELP_LEVELS] = "опыт: участие +5, победа выж +25, exe +30."; g_ru[EC_MSG_HELP_RANKS] = "ранги открываются с уровнями."; g_ru[EC_MSG_HELP_ABILITIES] = "способности: уровень 5 открывает персонажей, уровень 10 curse.exe."; g_ru[EC_MSG_HELP_COMMANDS] = ".menu .shop .inv .lang .maps";

	g_en[EC_MSG_REGISTER_SUCCESS] = "registered. use .login (password) to sign in.";
	g_en[EC_MSG_REGISTER_FAIL_EXISTS] = "this nickname is already registered.";
	g_en[EC_MSG_REGISTER_FAIL_SHORT] = "password too short (min 3).";
	g_en[EC_MSG_LOGIN_SUCCESS] = "logged in. permission level: %d.";
	g_en[EC_MSG_LOGIN_FAIL] = "wrong password or nickname not registered.";
	g_en[EC_MSG_LOGIN_USAGE] = CLRCODE_YLW ".login (password)" CLRCODE_RST;
	g_en[EC_MSG_REGISTER_USAGE] = CLRCODE_YLW ".register (password) (password)" CLRCODE_RST;
	g_en[EC_MSG_REG_STATUS_YES] = CLRCODE_YLW "account registered. use .login (password)" CLRCODE_RST;
	g_en[EC_MSG_REG_STATUS_NO] = CLRCODE_YLW "Account not registered. Please Register:" CLRCODE_RST;
	g_en[EC_MSG_AUTH_WARNING] = "account is password-protected. .login (password) within 10 seconds.";
	g_en[EC_MSG_AUTH_KICK] = "you did not log in in time.";
	g_en[EC_MSG_HINT_MENU] = "open menu: .menu / .m";
	g_en[EC_MSG_HINT_LANG] = "language: .lang";
	g_en[EC_MSG_NEED_AUTH] = "registered players only. .login / .register";
	g_en[EC_MSG_MENU_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "menu" CLRCODE_PUR " --------" CLRCODE_RST;
	g_en[EC_MSG_LEVEL_INFO] = "level: %d (%s)\nexp: %d\ncoins: %d\nwins surv/exe: %d/%d";
	g_en[EC_MSG_CHPASS_MISMATCH] = "passwords do not match.";
	g_en[EC_MSG_SKILL_LINE] = "skill: %s";
	g_en[EC_MSG_MENU_OPTION_LEVEL] = "level and stats"; g_en[EC_MSG_MENU_OPTION_SHOP] = "shop"; g_en[EC_MSG_MENU_OPTION_INVENTORY] = "inventory"; g_en[EC_MSG_MENU_OPTION_LANGUAGE] = "language"; g_en[EC_MSG_MENU_OPTION_HELP] = "help"; g_en[EC_MSG_MENU_OPTION_CHPASS] = "change password";
	g_en[EC_MSG_LANG_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "language" CLRCODE_PUR " --------" CLRCODE_RST; g_en[EC_MSG_LANG_CHANGED] = "language changed.";
	g_en[EC_MSG_SHOP_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "shop" CLRCODE_PUR " --------" CLRCODE_RST; g_en[EC_MSG_SHOP_DEMON] = "demon start (100)"; g_en[EC_MSG_SHOP_RINGS] = "red rings (30)"; g_en[EC_MSG_SHOP_SHIELD] = "totem of undying (50)"; g_en[EC_MSG_SHOP_LOTTERY] = "lottery (10)"; g_en[EC_MSG_SHOP_MAP] = "map select (20)";
	g_en[EC_MSG_SHOP_NO_COINS] = "not enough coins."; g_en[EC_MSG_SHOP_LIMIT] = "item limit: %d."; g_en[EC_MSG_LOTTERY_DEMON] = "lottery: demon start!"; g_en[EC_MSG_LOTTERY_RINGS] = "lottery: red rings!"; g_en[EC_MSG_LOTTERY_SHIELD] = "lottery: totem!"; g_en[EC_MSG_LOTTERY_REFUND] = "lottery: coins refunded."; g_en[EC_MSG_LOTTERY_LOSE] = "lottery: no prize.";
	g_en[EC_MSG_INVENTORY_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "inventory" CLRCODE_PUR " --------" CLRCODE_RST; g_en[EC_MSG_INVENTORY_EMPTY] = "inventory is empty."; g_en[EC_MSG_INV_DEMON] = "demon start x%d"; g_en[EC_MSG_INV_RINGS] = "red rings x%d"; g_en[EC_MSG_INV_SHIELD] = "totem of undying x%d"; g_en[EC_MSG_INV_MAP] = "map select x%d";
	g_en[EC_MSG_NO_ITEM] = "no item."; g_en[EC_MSG_DEMON_CONDITIONS] = "at least 4 players required."; g_en[EC_MSG_DEMON_USED] = "demon start is already used."; g_en[EC_MSG_DEMON_OK] = "demon start activated."; g_en[EC_MSG_RINGS_OK] = "red rings activated."; g_en[EC_MSG_SHIELD_OK] = "totem activated.";
	g_en[EC_MSG_MAPS_HEADER] = "choose a map:"; g_en[EC_MSG_MAPS_LINE] = "%d. %s"; g_en[EC_MSG_MAP_BAD_NUM] = "invalid map number."; g_en[EC_MSG_MAP_READY] = "cannot select a map during countdown."; g_en[EC_MSG_MAP_WAIT] = "%s selected %s.";
	g_en[EC_MSG_CHPASS_OLD] = "enter old password (0 cancels):"; g_en[EC_MSG_CHPASS_NEW] = "enter new password:"; g_en[EC_MSG_CHPASS_CONFIRM] = "confirm new password:"; g_en[EC_MSG_CHPASS_SUCCESS] = "password changed."; g_en[EC_MSG_CHPASS_FAIL] = "password change failed.";
	g_en[EC_MSG_HELP_LEVELS] = "exp: participation +5, survivor win +25, exe win +30."; g_en[EC_MSG_HELP_RANKS] = "ranks unlock with levels."; g_en[EC_MSG_HELP_ABILITIES] = "abilities: level 5 unlocks characters; level 10 curse.exe."; g_en[EC_MSG_HELP_COMMANDS] = ".menu .shop .inv .lang .maps";

	g_es[EC_MSG_REGISTER_SUCCESS] = "registrado. usa .login (contrasena).";
	g_es[EC_MSG_REGISTER_FAIL_EXISTS] = "este nick ya esta registrado.";
	g_es[EC_MSG_REGISTER_FAIL_SHORT] = "contrasena muy corta (min 3).";
	g_es[EC_MSG_LOGIN_SUCCESS] = "sesion iniciada. nivel de permisos: %d.";
	g_es[EC_MSG_LOGIN_FAIL] = "contrasena incorrecta o nick no registrado.";
	g_es[EC_MSG_LOGIN_USAGE] = CLRCODE_YLW ".login (contrasena)" CLRCODE_RST;
	g_es[EC_MSG_REGISTER_USAGE] = CLRCODE_YLW ".register (contrasena) (contrasena)" CLRCODE_RST;
	g_es[EC_MSG_REG_STATUS_YES] = CLRCODE_YLW "cuenta registrada. usa .login (contrasena)" CLRCODE_RST;
	g_es[EC_MSG_REG_STATUS_NO] = CLRCODE_YLW "Cuenta no registrada. Por favor registra:" CLRCODE_RST;
	g_es[EC_MSG_AUTH_WARNING] = "cuenta protegida. .login (contrasena) en 10 segundos.";
	g_es[EC_MSG_AUTH_KICK] = "no iniciaste sesion a tiempo.";
	g_es[EC_MSG_HINT_MENU] = "menu: .menu / .m";
	g_es[EC_MSG_HINT_LANG] = "idioma: .lang";
	g_es[EC_MSG_NEED_AUTH] = "solo registrados. .login / .register";
	g_es[EC_MSG_MENU_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "menu" CLRCODE_PUR " --------" CLRCODE_RST;
	g_es[EC_MSG_LEVEL_INFO] = "nivel: %d (%s)\nexp: %d\nmonedas: %d\nvictorias surv/exe: %d/%d";
	g_es[EC_MSG_CHPASS_MISMATCH] = "las contrasenas no coinciden.";
	g_es[EC_MSG_SKILL_LINE] = "skill: %s";
	g_es[EC_MSG_MENU_OPTION_LEVEL] = "nivel y estadisticas"; g_es[EC_MSG_MENU_OPTION_SHOP] = "tienda"; g_es[EC_MSG_MENU_OPTION_INVENTORY] = "inventario"; g_es[EC_MSG_MENU_OPTION_LANGUAGE] = "idioma"; g_es[EC_MSG_MENU_OPTION_HELP] = "ayuda"; g_es[EC_MSG_MENU_OPTION_CHPASS] = "cambiar contrasena";
	g_es[EC_MSG_LANG_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "idioma" CLRCODE_PUR " --------" CLRCODE_RST; g_es[EC_MSG_LANG_CHANGED] = "idioma cambiado.";
	g_es[EC_MSG_SHOP_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "tienda" CLRCODE_PUR " --------" CLRCODE_RST; g_es[EC_MSG_SHOP_DEMON] = "demonio al inicio (100)"; g_es[EC_MSG_SHOP_RINGS] = "anillos rojos (30)"; g_es[EC_MSG_SHOP_SHIELD] = "totem de inmortalidad (50)"; g_es[EC_MSG_SHOP_LOTTERY] = "loteria (10)"; g_es[EC_MSG_SHOP_MAP] = "seleccionar mapa (20)";
	g_es[EC_MSG_SHOP_NO_COINS] = "monedas insuficientes."; g_es[EC_MSG_SHOP_LIMIT] = "limite de objetos: %d."; g_es[EC_MSG_LOTTERY_DEMON] = "loteria: demonio!"; g_es[EC_MSG_LOTTERY_RINGS] = "loteria: anillos rojos!"; g_es[EC_MSG_LOTTERY_SHIELD] = "loteria: totem!"; g_es[EC_MSG_LOTTERY_REFUND] = "loteria: monedas devueltas."; g_es[EC_MSG_LOTTERY_LOSE] = "loteria: sin premio.";
	g_es[EC_MSG_INVENTORY_HEADER] = CLRCODE_PUR "-------- " CLRCODE_YLW "inventario" CLRCODE_PUR " --------" CLRCODE_RST; g_es[EC_MSG_INVENTORY_EMPTY] = "inventario vacio."; g_es[EC_MSG_INV_DEMON] = "demonio al inicio x%d"; g_es[EC_MSG_INV_RINGS] = "anillos rojos x%d"; g_es[EC_MSG_INV_SHIELD] = "totem x%d"; g_es[EC_MSG_INV_MAP] = "seleccionar mapa x%d";
	g_es[EC_MSG_NO_ITEM] = "sin objeto."; g_es[EC_MSG_DEMON_CONDITIONS] = "se necesitan 4 jugadores."; g_es[EC_MSG_DEMON_USED] = "demonio ya usado."; g_es[EC_MSG_DEMON_OK] = "demonio activado."; g_es[EC_MSG_RINGS_OK] = "anillos rojos activados."; g_es[EC_MSG_SHIELD_OK] = "totem activado.";
	g_es[EC_MSG_MAPS_HEADER] = "elige un mapa:"; g_es[EC_MSG_MAPS_LINE] = "%d. %s"; g_es[EC_MSG_MAP_BAD_NUM] = "numero de mapa invalido."; g_es[EC_MSG_MAP_READY] = "no puedes elegir mapa durante cuenta atras."; g_es[EC_MSG_MAP_WAIT] = "%s eligio %s.";
	g_es[EC_MSG_CHPASS_OLD] = "contrasena anterior (0 cancela):"; g_es[EC_MSG_CHPASS_NEW] = "nueva contrasena:"; g_es[EC_MSG_CHPASS_CONFIRM] = "confirma contrasena:"; g_es[EC_MSG_CHPASS_SUCCESS] = "contrasena cambiada."; g_es[EC_MSG_CHPASS_FAIL] = "no se pudo cambiar contrasena.";
	g_es[EC_MSG_HELP_LEVELS] = "exp: participar +5, ganar superviviente +25, exe +30."; g_es[EC_MSG_HELP_RANKS] = "rangos se desbloquean con niveles."; g_es[EC_MSG_HELP_ABILITIES] = "habilidades: nivel 5 personajes; nivel 10 curse.exe."; g_es[EC_MSG_HELP_COMMANDS] = ".menu .shop .inv .lang .maps";
}

const char* ec_msg(ECMsgId id, const char* lang)
{
	if (id < 0 || id >= EC_MSG_COUNT)
		return "???";
	if (lang && strcmp(lang, "es") == 0 && g_es[id])
		return g_es[id];
	if (lang && strcmp(lang, "en") == 0 && g_en[id])
		return g_en[id];
	return g_ru[id] ? g_ru[id] : "???";
}

const char* ec_lang_pick(const char* lang, const char* ru, const char* en, const char* es)
{
	if (lang && strcmp(lang, "es") == 0)
		return es;
	if (lang && strcmp(lang, "en") == 0)
		return en;
	return ru;
}

const char* ec_title_for_level(uint8_t level, const char* lang)
{
	if (level > 20)
		level = 20;
	if (lang && strcmp(lang, "en") == 0)
		return titles_en[level];
	if (lang && strcmp(lang, "es") == 0)
		return titles_es[level];
	return titles_ru[level];
}
