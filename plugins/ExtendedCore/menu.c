#include "menu.h"
#include "accounts.h"
#include "locale.h"
#include "peer.h"
#include <Colors.h>
#include <Lib.h>
#include <Maps.h>
#include <States.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MENU_LINES 8
#define SHOP_PER_PAGE 3
#define MAPS_PER_PAGE 4

typedef struct { int vanilla_id; int8_t map_index; const char* label; } ECMenuMap;
static const ECMenuMap g_maps[] = {
	{1,14,"hide and seek"},{2,1,"ravine myst"},{3,2,"..."},{4,3,"desert town"},
	{5,4,"you cant run"},{6,5,"limp city"},{7,6,"not perfect"},{8,7,"kind and fair"},
	{9,8,"____________"},{10,9,"nasty paradise"},{11,10,"priceless freedom"},
	{12,11,"volcano valley"},{13,12,"hill"},{14,13,"majin forest zone"},
	{15,0,"hide and seek 2"},{16,15,"torture cave"},{17,16,"dark tower zone"},
	{18,17,"haunting zone"},{19,18,"mystic wood"},{20,19,"echidna ruins zone"}
};
#define MENU_MAP_COUNT ((int)(sizeof(g_maps) / sizeof(g_maps[0])))

static ECPeer* ec_peer(PeerData* v, int id) { return (ECPeer*)plugin_peer_udata(v, id); }
static ECServer* ec_server(Server* s, int id) { return (ECServer*)plugin_server_udata(s, id); }
static const char* lang(ECPeer* ec) { return ec->language[0] ? ec->language : "en"; }
static void say(PeerData* v, const PluginHost* h, const char* text) { if (h && h->send_msg) h->send_msg(v->server, v->peer, text); }
static void save_progress(ECPeer* ec) { ec_account_set_progress(ec->login, ec->exp, ec->level, ec->coins); }

static void clear(PeerData* v, const PluginHost* h)
{
	for (int i = 0; i < MENU_LINES; i++) say(v, h, " ");
}
static void option(PeerData* v, const PluginHost* h, int number, const char* text)
{
	char line[160]; snprintf(line, sizeof(line), CLRCODE_YLW "%d." CLRCODE_RST " %s", number, text); say(v, h, line);
}
static void menu_nav(PeerData* v, ECPeer* ec, const PluginHost* h, bool prev, bool next)
{
	char line[160];
	snprintf(line, sizeof(line), "0 %s | %s8 %s | %s9 %s", ec_lang_pick(lang(ec), "выход", "exit", "salir"),
		prev ? "" : CLRCODE_GRA, ec_lang_pick(lang(ec), "назад", "prev", "ant"),
		next ? "" : CLRCODE_GRA, ec_lang_pick(lang(ec), "далее", "next", "sig"));
	say(v, h, line);
}
static void open_main(PeerData*, ECPeer*, const PluginHost*, int);
static void open_inventory(PeerData*, ECPeer*, const PluginHost*, int);
static void open_shop(PeerData*, ECPeer*, const PluginHost*, int, unsigned);
static void open_maps(PeerData*, ECPeer*, const PluginHost*, int, unsigned);
static void open_help(PeerData*, ECPeer*, const PluginHost*, int, unsigned);

static void close_menu(ECPeer* ec)
{
	ec->menu_screen = EC_MENU_NONE; ec->menu_muted = false; ec->menu_chpass_step = 0;
	ec->menu_chpass_old[0] = ec->menu_chpass_new[0] = '\0';
	memset(ec->menu_inv_map, 0, sizeof(ec->menu_inv_map));
}
static void open_screen(ECPeer* ec, MenuScreen screen) { ec->menu_screen = screen; ec->menu_muted = true; }

static void open_main(PeerData* v, ECPeer* ec, const PluginHost* h, int id)
{
	(void)id;
	if (!ec->authenticated) { say(v, h, ec_msg(EC_MSG_NEED_AUTH, lang(ec))); return; }
	open_screen(ec, EC_MENU_MAIN); clear(v,h); say(v,h,ec_msg(EC_MSG_MENU_HEADER,lang(ec)));
	option(v,h,1,ec_msg(EC_MSG_MENU_OPTION_LEVEL,lang(ec))); option(v,h,2,ec_msg(EC_MSG_MENU_OPTION_SHOP,lang(ec)));
	option(v,h,3,ec_msg(EC_MSG_MENU_OPTION_INVENTORY,lang(ec))); option(v,h,4,ec_msg(EC_MSG_MENU_OPTION_LANGUAGE,lang(ec)));
	option(v,h,5,ec_msg(EC_MSG_MENU_OPTION_HELP,lang(ec))); option(v,h,6,ec_msg(EC_MSG_MENU_OPTION_CHPASS,lang(ec))); menu_nav(v,ec,h,false,false);
}
static void open_level(PeerData* v, ECPeer* ec, const PluginHost* h)
{
	char b[160]; open_screen(ec,EC_MENU_LEVEL); clear(v,h); say(v,h,ec_msg(EC_MSG_MENU_HEADER,lang(ec)));
	snprintf(b,sizeof(b),"level: %u (%s)",ec->level,ec_title_for_level(ec->level,lang(ec))); say(v,h,b);
	snprintf(b,sizeof(b),"exp: %u | coins: %u",ec->exp,ec->coins); say(v,h,b);
	snprintf(b,sizeof(b),"wins surv/exe: %u/%u",ec->wins_surv,ec->wins_exe); say(v,h,b); menu_nav(v,ec,h,true,false);
}
static void open_language(PeerData* v, ECPeer* ec, const PluginHost* h, bool from_main)
{
	ec->menu_lang_from_main=from_main; open_screen(ec,EC_MENU_LANG); clear(v,h); say(v,h,ec_msg(EC_MSG_LANG_HEADER,lang(ec)));
	option(v,h,1,"Russian"); option(v,h,2,"English"); option(v,h,3,"Espanol"); menu_nav(v,ec,h,true,false);
}
static const ECMsgId shop_msg(int item) {
	static const ECMsgId ids[] = {EC_MSG_SHOP_DEMON,EC_MSG_SHOP_RINGS,EC_MSG_SHOP_SHIELD,EC_MSG_SHOP_LOTTERY,EC_MSG_SHOP_MAP};
	return ids[item - 1];
}
static void open_shop(PeerData* v, ECPeer* ec, const PluginHost* h, int id, unsigned page)
{
	const unsigned pages=2; if(page>=pages) page=pages-1; ec->menu_shop_page=(uint8_t)page; open_screen(ec,EC_MENU_SHOP); clear(v,h); say(v,h,ec_msg(EC_MSG_SHOP_HEADER,lang(ec)));
	char b[64]; snprintf(b,sizeof(b),"coins: %u | page %u/%u",ec->coins,page+1,pages); say(v,h,b); memset(ec->menu_shop_map,0,sizeof(ec->menu_shop_map));
	for(int i=0;i<SHOP_PER_PAGE;i++){int item=(int)page*SHOP_PER_PAGE+i+1;if(item>5)break;ec->menu_shop_map[i+1]=(int8_t)item;option(v,h,i+1,ec_msg(shop_msg(item),lang(ec)));}
	menu_nav(v,ec,h,page>0,page+1<pages); (void)id;
}
static void open_inventory(PeerData* v, ECPeer* ec, const PluginHost* h, int id)
{
	ECItemType items[] = {EC_ITEM_DEMON_START,EC_ITEM_RED_RINGS,EC_ITEM_SHIELD,EC_ITEM_MAP_SELECT};
	ECMsgId names[] = {EC_MSG_INV_DEMON,EC_MSG_INV_RINGS,EC_MSG_INV_SHIELD,EC_MSG_INV_MAP};
	int slot=0; open_screen(ec,EC_MENU_INVENTORY); clear(v,h); say(v,h,ec_msg(EC_MSG_INVENTORY_HEADER,lang(ec))); memset(ec->menu_inv_map,0,sizeof(ec->menu_inv_map));
	for(int i=0;i<4;i++){int count=0;ec_account_get_item_count(ec->login,items[i],&count);if(count>0){char b[128];snprintf(b,sizeof(b),ec_msg(names[i],lang(ec)),count);ec->menu_inv_map[slot]=items[i];option(v,h,slot+1,b);slot++;}}
	if(!slot) say(v,h,ec_msg(EC_MSG_INVENTORY_EMPTY,lang(ec))); menu_nav(v,ec,h,true,false); (void)id;
}
static void open_maps(PeerData* v, ECPeer* ec, const PluginHost* h, int id, unsigned page)
{
	unsigned pages=(MENU_MAP_COUNT+MAPS_PER_PAGE-1)/MAPS_PER_PAGE; if(page>=pages)page=pages-1; ec->menu_map_page=(uint8_t)page; open_screen(ec,EC_MENU_MAP_SELECT); clear(v,h); say(v,h,ec_msg(EC_MSG_MAPS_HEADER,lang(ec)));
	for(int i=0;i<MAPS_PER_PAGE;i++){int n=(int)page*MAPS_PER_PAGE+i;if(n>=MENU_MAP_COUNT)break;char b[128];snprintf(b,sizeof(b),ec_msg(EC_MSG_MAPS_LINE,lang(ec)),g_maps[n].vanilla_id,g_maps[n].label);say(v,h,b);} menu_nav(v,ec,h,page>0,page+1<pages);(void)id;
}
static void open_help(PeerData* v, ECPeer* ec, const PluginHost* h, int id, unsigned tab)
{
	static const ECMsgId content[] = {EC_MSG_HELP_LEVELS,EC_MSG_HELP_RANKS,EC_MSG_HELP_ABILITIES,EC_MSG_HELP_COMMANDS}; if(tab>3)tab=0;ec->menu_help_tab=(uint8_t)tab;open_screen(ec,EC_MENU_HELP);clear(v,h);
	say(v,h,ec_lang_pick(lang(ec),CLRCODE_PUR "-------- " CLRCODE_YLW "помощь" CLRCODE_RST,CLRCODE_PUR "-------- " CLRCODE_YLW "help" CLRCODE_RST,CLRCODE_PUR "-------- " CLRCODE_YLW "ayuda" CLRCODE_RST));
	say(v,h,"1 levels | 2 ranks | 3 abilities | 4 commands"); say(v,h,ec_msg(content[tab],lang(ec))); menu_nav(v,ec,h,true,false);(void)id;
}

static bool buy(PeerData* v, ECPeer* ec, const PluginHost* h, int item)
{
	int price=0,count=0; ECItemType type=EC_ITEM_NONE;
	if(item==1){price=100;type=EC_ITEM_DEMON_START;} else if(item==2){price=30;type=EC_ITEM_RED_RINGS;} else if(item==3){price=50;type=EC_ITEM_SHIELD;} else if(item==5){price=20;type=EC_ITEM_MAP_SELECT;}
	if(item==4){ price=10; if(ec->coins<10){say(v,h,ec_msg(EC_MSG_SHOP_NO_COINS,lang(ec)));return false;} ec->coins-=10;int r=rand()%100;ECMsgId m=EC_MSG_LOTTERY_LOSE;if(r<5){ec_account_add_item(ec->login,EC_ITEM_DEMON_START,1);m=EC_MSG_LOTTERY_DEMON;}else if(r<20){ec_account_add_item(ec->login,EC_ITEM_RED_RINGS,1);m=EC_MSG_LOTTERY_RINGS;}else if(r<40){ec_account_add_item(ec->login,EC_ITEM_SHIELD,1);m=EC_MSG_LOTTERY_SHIELD;}else if(r<70){ec->coins+=10;m=EC_MSG_LOTTERY_REFUND;}save_progress(ec);say(v,h,ec_msg(m,lang(ec)));return true;}
	if(type==EC_ITEM_NONE)return false;ec_account_get_item_count(ec->login,type,&count);if(count>=EC_MAX_INV_ITEMS){char b[96];snprintf(b,sizeof(b),ec_msg(EC_MSG_SHOP_LIMIT,lang(ec)),EC_MAX_INV_ITEMS);say(v,h,b);return false;}if(ec->coins<(uint32_t)price){say(v,h,ec_msg(EC_MSG_SHOP_NO_COINS,lang(ec)));return false;}
	ec->coins-=(uint32_t)price;save_progress(ec);ec_account_add_item(ec->login,type,1);return true;
}
static bool use_item(PeerData* v,ECPeer* ec,const PluginHost* h,int id,ECItemType item)
{
	int count=0;ec_account_get_item_count(ec->login,item,&count);if(count<=0){say(v,h,ec_msg(EC_MSG_NO_ITEM,lang(ec)));return false;}
	if(item==EC_ITEM_MAP_SELECT){open_maps(v,ec,h,id,0);return true;}
	if(item==EC_ITEM_DEMON_START){ECServer* s=ec_server(v->server,id);if(!h->ingame_count||h->ingame_count(v->server)<4){say(v,h,ec_msg(EC_MSG_DEMON_CONDITIONS,lang(ec)));return false;}if(!s||s->demon_start_used){say(v,h,ec_msg(EC_MSG_DEMON_USED,lang(ec)));return false;}s->demon_start_used=true;s->demon_start_player=v->id;say(v,h,ec_msg(EC_MSG_DEMON_OK,lang(ec)));}
	if(item==EC_ITEM_RED_RINGS){ECServer* s=ec_server(v->server,id);if(s)s->red_ring_boost=true;say(v,h,ec_msg(EC_MSG_RINGS_OK,lang(ec)));}
	if(item==EC_ITEM_SHIELD){ec->shield_active=true;say(v,h,ec_msg(EC_MSG_SHIELD_OK,lang(ec)));}
	return ec_account_remove_item(ec->login,item,1);
}

void ec_menu_init(void) { srand((unsigned int)time(NULL)); }
bool ec_menu_is_muted(PeerData* v, int plugin_id) { ECPeer* ec=ec_peer(v,plugin_id);return ec&&ec->menu_muted; }
void ec_menu_open_main(PeerData* v,const PluginHost* h,int id){ECPeer* ec=ec_peer(v,id);if(ec)open_main(v,ec,h,id);}

bool ec_menu_handle_chat(PeerData* v,String* msg,const PluginHost* h,int id)
{
	ECPeer* ec=ec_peer(v,id);if(!ec||!h)return false;const char* text=msg->value;
	if(!ec->menu_muted){if(!strcmp(text,".menu")||!strcmp(text,".m")||!strcmp(text,".mn")){open_main(v,ec,h,id);return true;}if(!strcmp(text,".shop")){open_shop(v,ec,h,id,0);return true;}if(!strcmp(text,".inv")||!strcmp(text,".inventory")||!strcmp(text,".i")){open_inventory(v,ec,h,id);return true;}if(!strcmp(text,".lang")||!strcmp(text,".language")){open_language(v,ec,h,false);return true;}if(!strcmp(text,".maps")||!strcmp(text,".smap")){open_maps(v,ec,h,id,0);return true;}return false;}
	if(ec->menu_screen==EC_MENU_CHPASS){if(!strcmp(text,"0")){close_menu(ec);return true;}if(ec->menu_chpass_step==1){strncpy(ec->menu_chpass_old,text,sizeof(ec->menu_chpass_old)-1);ec->menu_chpass_step=2;say(v,h,ec_msg(EC_MSG_CHPASS_NEW,lang(ec)));}else if(ec->menu_chpass_step==2){strncpy(ec->menu_chpass_new,text,sizeof(ec->menu_chpass_new)-1);ec->menu_chpass_step=3;say(v,h,ec_msg(EC_MSG_CHPASS_CONFIRM,lang(ec)));}else{bool ok=!strcmp(text,ec->menu_chpass_new)&&ec_account_change_password(ec->login,ec->menu_chpass_old,ec->menu_chpass_new);say(v,h,ec_msg(ok?EC_MSG_CHPASS_SUCCESS:EC_MSG_CHPASS_FAIL,lang(ec)));close_menu(ec);}return true;}
	int n=-1;if(sscanf(text,"%d",&n)!=1)return true;if(n==0){close_menu(ec);return true;}if(n==8){if(ec->menu_screen==EC_MENU_SHOP&&ec->menu_shop_page)open_shop(v,ec,h,id,ec->menu_shop_page-1);else if(ec->menu_screen==EC_MENU_MAP_SELECT&&ec->menu_map_page)open_maps(v,ec,h,id,ec->menu_map_page-1);else open_main(v,ec,h,id);return true;}if(n==9){if(ec->menu_screen==EC_MENU_SHOP)open_shop(v,ec,h,id,ec->menu_shop_page+1);else if(ec->menu_screen==EC_MENU_MAP_SELECT)open_maps(v,ec,h,id,ec->menu_map_page+1);return true;}
	switch(ec->menu_screen){case EC_MENU_MAIN:if(n==1)open_level(v,ec,h);else if(n==2)open_shop(v,ec,h,id,0);else if(n==3)open_inventory(v,ec,h,id);else if(n==4)open_language(v,ec,h,true);else if(n==5)open_help(v,ec,h,id,0);else if(n==6){ec->menu_screen=EC_MENU_CHPASS;ec->menu_chpass_step=1;say(v,h,ec_msg(EC_MSG_CHPASS_OLD,lang(ec)));}break;case EC_MENU_LANG:if(n>=1&&n<=3){const char* codes[]={"ru","en","es"};strcpy(ec->language,codes[n-1]);ec_account_set_language(ec->login,ec->language);say(v,h,ec_msg(EC_MSG_LANG_CHANGED,lang(ec)));open_language(v,ec,h,ec->menu_lang_from_main);}break;case EC_MENU_SHOP:if(n>=1&&n<=3&&buy(v,ec,h,ec->menu_shop_map[n])){ec->menu_shop_bought_slot=(uint8_t)n;ec->menu_shop_bought_timer=0.5*TICKSPERSEC;ec->menu_shop_bought_fresh=true;}open_shop(v,ec,h,id,ec->menu_shop_page);break;case EC_MENU_INVENTORY:if(n>=1&&n<=EC_MENU_INV_SLOTS&&ec->menu_inv_map[n-1]){if(use_item(v,ec,h,id,ec->menu_inv_map[n-1])&&ec->menu_screen!=EC_MENU_MAP_SELECT)close_menu(ec);}break;case EC_MENU_MAP_SELECT:for(int i=0;i<MENU_MAP_COUNT;i++)if(g_maps[i].vanilla_id==n){if(ec_account_remove_item(ec->login,EC_ITEM_MAP_SELECT,1)){v->server->last_map=g_maps[i].map_index;charselect_init(g_maps[i].map_index,v->server);close_menu(ec);}else say(v,h,ec_msg(EC_MSG_NO_ITEM,lang(ec)));break;}break;case EC_MENU_HELP:if(n>=1&&n<=4)open_help(v,ec,h,id,n-1);break;default:break;}return true;
}
void ec_menu_tick(Server* server,double delta,const PluginHost* h,int id){if(!server)return;for(size_t i=0;i<server->peers.capacity;i++){PeerData* v=(PeerData*)server->peers.ptr[i];ECPeer* ec=v?ec_peer(v,id):NULL;if(ec&&ec->menu_shop_bought_timer>0){ec->menu_shop_bought_timer-=delta;if(ec->menu_shop_bought_timer<0)ec->menu_shop_bought_timer=0;}}(void)h;}
