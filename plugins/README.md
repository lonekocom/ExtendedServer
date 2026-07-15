# Руководство по разработке плагинов ExtendedServer

Документ для авторов плагинов: API, жизненный цикл, userdata, сборка сервера и подключение своего кода.

**Cервер можно собрать без ExtendedCore.**  
Опция CMake `WITH_EXTENDED_CORE` по умолчанию `ON`. Для «чистого» AdvancedServer:

```bash
cmake .. -DWITH_EXTENDED_CORE=OFF
```

В этом режиме плагины из `plugins/ExtendedCore` не линкуются. Ядро и plugin API остаются; список плагинов при старте будет пустым (или только динамические `.dll`/`.so`, если включён `PLUGIN_DYNAMIC_LOADING`).

---

## 1. Общая модель

Плагин — модуль на C, который:

1. Регистрируется через `plugin_register()` с `PluginInfo` (имя, версия, размер userdata, таблица хуков).
2. Получает `PluginHost` — функции хоста (сообщения, пакеты, поиск пиров, disconnect и т.д.).
3. Хранит своё состояние в **userdata** на peer и/или server (не патчит `PeerData`/`Server` в публичных заголовках ядра).
4. В хуках возвращает:
   - `PLUGIN_CONTINUE` — ядро и следующие плагины продолжают работу;
   - `PLUGIN_HANDLED` — цепочка останавливается; там, где это предусмотрено, stock-обработчик **пропускается**.

Заголовок API: [`include/Plugin.h`](../include/Plugin.h).  
Текущая версия API: **`PLUGIN_API_VERSION 2`**.  
Лимит одновременно зарегистрированных плагинов: **`PLUGIN_MAX` (16)**.

При старте сервер печатает список:

```text
--------- Plugins (N) ---------
  [0] ExtendedCore v1.0.0
  [1] MyPlugin v0.1.0
--------------------------------
```

---

## 2. Два способа подключить плагин

### 2.1. Статическая линковка (рекомендуется для своих форков)

Исходники плагина компилируются **в тот же** бинарник `AdvancedServer`.

- ExtendedCore уже так подключён при `WITH_EXTENDED_CORE=ON`.
- Свой плагин: добавьте `.c` в `advancedserver/CMakeLists.txt` рядом с ExtendedCore **или** заведите отдельную CMake-цель и линкуйте её к `AdvancedServer`.
- Точка входа вызывается из ядра при `#ifdef WITH_YOUR_PLUGIN` (по аналогии с `extendedcore_plugin_entry` в `Plugin.c`).

Плюсы: проще отладка, нет `LoadLibrary`/`dlopen`.  
Минусы: нужна пересборка сервера при каждом изменении плагина.

### 2.2. Динамическая загрузка (`.dll` / `.so`)

Включите:

```bash
cmake .. -DPLUGIN_DYNAMIC_LOADING=ON
```

При `plugins_init()` сервер сканирует каталог `plugins/` рядом с рабочей директорией процесса и загружает:

| ОС | Маска |
|----|--------|
| Windows | `plugins\*.dll` |
| Linux/Unix | `plugins/*.so` |

Каждая библиотека должна экспортировать:

```c
PLUGIN_EXPORT bool plugin_entry(const PluginHost* host);
```

Внутри `plugin_entry` вызовите `plugin_register(...)` и верните `true` при успехе.

Плюсы: можно менять плагины без пересборки ядра (при совместимом API).  
Минусы: нужна совместимость ABI/`PLUGIN_API_VERSION`, корректный runtime path, осторожность с заголовками `Server.h` / `Player.h`.

---

## 3. Минимальный плагин (шаблон)

```c
#include <Plugin.h>
#include <Log.h>
#include <Server.h>
#include <string.h>

typedef struct {
    int join_count;
} MyPeerData;

static const PluginHost* g_host = NULL;
static int g_id = -1;

static void on_init(const PluginHost* host)
{
    g_host = host;
    Info("MyPlugin: init (API %d)", host->api_version);
}

static void on_shutdown(void)
{
    Info("MyPlugin: shutdown");
    g_host = NULL;
}

static void on_peer_join(PeerData* peer)
{
    MyPeerData* ud = (MyPeerData*)g_host->peer_udata(peer, g_id);
    if (!ud)
        return;
    ud->join_count++;
    g_host->send_msg(peer->server, peer->peer, "welcome from MyPlugin");
}

static PluginResult on_chat(PeerData* peer, String* msg)
{
    if (strncmp(msg->value, ".hello", 6) == 0) {
        g_host->send_msg(peer->server, peer->peer, "hello!");
        return PLUGIN_HANDLED; /* не отдавать сообщение в stock-чат */
    }
    return PLUGIN_CONTINUE;
}

bool myplugin_entry(const PluginHost* host)
{
    (void)host;
    PluginInfo info = {
        .name = "MyPlugin",
        .version = "0.1.0",
        .api_version = PLUGIN_API_VERSION,
        .peer_data_size = sizeof(MyPeerData),
        .server_data_size = 0,
        .hooks = {
            .on_init = on_init,
            .on_shutdown = on_shutdown,
            .on_peer_join = on_peer_join,
            .on_chat = on_chat,
        }
    };
    g_id = plugin_register(&info);
    return g_id >= 0;
}

/* Для динамической загрузки: */
PLUGIN_EXPORT bool plugin_entry(const PluginHost* host)
{
    return myplugin_entry(host);
}
```

Незаполненные указатели в `PluginHooks` можно оставить `NULL` — ядро их не вызывает.

---

## 4. Регистрация и userdata

### `PluginInfo`

| Поле | Назначение |
|------|------------|
| `name` | Уникальное имя (дубликаты отклоняются) |
| `version` | Строка для лога |
| `api_version` | Должен быть равен `PLUGIN_API_VERSION` хоста |
| `peer_data_size` | Байты на каждого peer (`calloc` при connect) |
| `server_data_size` | Байты на каждый `Server` |
| `hooks` | Таблица колбэков |

`plugin_register` возвращает **id** `0 … PLUGIN_MAX-1` или `-1`.

### Доступ к userdata

```c
void* ud = host->peer_udata(peer, plugin_id);
void* sd = host->server_udata(server, plugin_id);
```

Либо напрямую:

```c
plugin_peer_udata(peer, plugin_id);
plugin_server_udata(server, plugin_id);
```

Ядро выделяет и обнуляет блоки; плагин **не** должен `free` их сам (освобождает host при disconnect / destroy server).

Храните в userdata всё кастомное: меню, аккаунты, таймеры абилок, флаги flood и т.д. Не расширяйте публичные структуры ядра в чужих плагинах без согласования с мейнтейнерами.

---

## 5. PluginHost — что можно вызывать

| Функция | Назначение |
|---------|------------|
| `send_msg` | Личное сообщение в чат клиенту |
| `broadcast_msg` | Чат всем (sender id, `0` = сервер) |
| `send_packet` / `broadcast_packet` / `broadcast_packet_ex` | Сырые пакеты |
| `find_peer` | Peer по id |
| `peer_count` / `ingame_count` | Счётчики |
| `peer_udata` / `server_udata` | Слоты плагина |
| `disconnect` | Кик (`DisconnectReason` из `Lib.h`) |
| `get_server` / `server_count` | Мульти-лобби инстансы |

`host->api_version` сверяйте с `#define PLUGIN_API_VERSION`.

Типы `Server`, `PeerData`, `Player`, `Packet`, состояния (`ST_LOBBY`, `ST_GAME`, …) — из заголовков ядра (`Server.h`, `Player.h`, `Packet.h`, `States.h`). Плагин, который лезет глубоко в геймплей, должен включать их явно.

---

## 6. Хуки (жизненный цикл)

Порядок важен для понимания, **когда** срабатывает колбэк и что значит `PLUGIN_HANDLED`.

### Глобальные

| Хук | Когда | HANDLED |
|-----|--------|---------|
| `on_init` | После регистрации всех плагинов, до создания серверов | — |
| `on_shutdown` | При остановке | — |

### Сервер

| Хук | Когда |
|-----|--------|
| `on_server_created` | После создания `Server` + userdata |
| `on_server_destroyed` | Перед освобождением userdata |

### Peer

| Хук | Когда | HANDLED |
|-----|--------|---------|
| `on_peer_connect` | Сразу после `malloc` PeerData | — |
| `on_peer_disconnect` | Перед free peer | — |
| `on_peer_identity` | Поля identity заполнены, **до** stock join | Пропускает `peer_identity_process` (плагин сам отвечает за join/kick) |
| `on_peer_identity_ok` | Успешный join | — |
| `on_peer_join` | После state-join | — |
| `on_peer_leave` | Перед state-leave | — |

### Пакеты / чат / команды

| Хук | Когда | HANDLED |
|-----|--------|---------|
| `on_packet` | До `server_state_handle` | Stock-обработчик пакета не вызывается |
| `on_chat` | После проверки длины чата (lobby/results) | Команды + broadcast stock не выполняются |
| `on_command` | Stock `server_cmd_handle` не знает команду | Команда считана обработанной |

### Тик и состояния

| Хук | Когда |
|-----|--------|
| `on_tick` | Каждый тик worker-потока (после state tick) |
| `on_state_change` | Через `server_set_state` (lobby/mapvote/char/game/results) |
| `on_game_start` | Вход в игру |
| `on_game_end` | Конец раунда (ending = `Ending`) |
| `on_results` | Вход в results |

### Геймплей (API v2)

| Хук | Когда | HANDLED |
|-----|--------|---------|
| `on_player_data` | После stock `CLIENT_PLAYER_DATA` (позиция/кольца/state) | — |
| `on_game_player_tick` | На каждого in-game peer после stock player tick | — |
| `on_player_death` | Перед установкой `PLAYER_DEAD` | Отмена смерти (щит и т.п.) |

Несколько плагинов вызываются **по порядку регистрации**. Первый `PLUGIN_HANDLED` обрывает цепочку для этого события.

---

## 7. Практические правила

1. **Не блокируйте** worker надолго в хуках (файловый I/O — с осторожностью; лучше кэш).
2. **Потокобезопасность:** каждый `Server` крутится в своём потоке; глобальные данные (accounts.json и т.п.) защищайте мьютексом.
3. **Чат:** если вернули `HANDLED`, сами отправьте ответ/`broadcast`, иначе игрок ничего не увидит.
4. **Identity HANDLED:** вы полностью берёте на себя проверку и join — ошибиться легко; чаще хватает `on_peer_identity_ok` + `on_chat`.
5. **Совместимость API:** при повышении `PLUGIN_API_VERSION` пересоберите плагины. Статический ExtendedCore всегда совпадает с деревом репозитория.
6. Смотрите эталон: [`plugins/ExtendedCore/`](./ExtendedCore/) (`plugin_main.c` + модули accounts/menu/abilities/…).

---

## 8. Сборка проекта

### Требования

- CMake ≥ 3.16.3  
- Компилятор C (MinGW/GCC, Clang или MSVC)  
- Зависимости как у обычного AdvancedServer (enet в дереве; SDL только при `BUILD_UI`)

### Windows (MinGW), пример

```powershell
cd C:\projects\ExtendedServer
mkdir build-windows -Force
cd build-windows

# С ExtendedCore (по умолчанию)
cmake .. -G "MinGW Makefiles" -DWITH_EXTENDED_CORE=ON
cmake --build . --target AdvancedServer -j 8

# Без ExtendedCore — чистый сервер + пустой список плагинов
cmake .. -G "MinGW Makefiles" -DWITH_EXTENDED_CORE=OFF
cmake --build . --target AdvancedServer -j 8

# Динамические плагины из папки plugins/*.dll
cmake .. -G "MinGW Makefiles" -DWITH_EXTENDED_CORE=OFF -DPLUGIN_DYNAMIC_LOADING=ON
cmake --build . --target AdvancedServer -j 8
```

Бинарник: `build-windows/advancedserver/AdvancedServer.exe`  
Рабочая директория при запуске должна содержать `Config` / `Player_Data` по необходимости (ExtendedCore пишет `Player_Data/Accounts.json`).

### Linux

```bash
mkdir -p build && cd build
cmake .. -DWITH_EXTENDED_CORE=ON   # или OFF
cmake --build . --target AdvancedServer -j$(nproc)
```

При `PLUGIN_DYNAMIC_LOADING=ON` на Unix линкуется `libdl` (`${CMAKE_DL_LIBS}`).

### Полезные CMake-опции

| Опция | По умолчанию | Смысл |
|-------|--------------|--------|
| `WITH_EXTENDED_CORE` | `ON` | Статически вшить ExtendedCore |
| `PLUGIN_DYNAMIC_LOADING` | `OFF` | Грузить `plugins/*.dll` / `*.so` |
| `BUILD_UI` | `OFF` | SDL UI |
| `DYLIB` | `OFF` | Собрать весь сервер как shared library (это **не** система плагинов) |

---

## 9. Как добавить свой статический плагин в репозиторий

1. Создайте каталог, например `plugins/MyPlugin/`.
2. Реализуйте `plugin_main.c` с `myplugin_entry` + `plugin_register`.
3. В [`advancedserver/CMakeLists.txt`](../advancedserver/CMakeLists.txt) добавьте опцию и источники по образцу ExtendedCore:

```cmake
option(WITH_MY_PLUGIN "Statically link MyPlugin" ON)

if(WITH_MY_PLUGIN)
  add_compile_definitions(WITH_MY_PLUGIN)
  list(APPEND SOURCES
    "${CMAKE_SOURCE_DIR}/plugins/MyPlugin/plugin_main.c"
  )
  include_directories("${CMAKE_SOURCE_DIR}/plugins/MyPlugin")
endif()
```

4. В [`advancedserver/Plugin.c`](../advancedserver/Plugin.c) в `plugins_init()`:

```c
#ifdef WITH_MY_PLUGIN
extern bool myplugin_entry(const PluginHost* host);
if (!myplugin_entry(&g_host))
    Warn("MyPlugin failed to register");
#endif
```

5. Пересоберите.

---

## 10. Как собрать динамический `.dll` / `.so`

Отдельный CMakeLists плагина (упрощённо):

```cmake
add_library(MyPlugin SHARED plugin_main.c)
target_include_directories(MyPlugin PRIVATE
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/enet/include
)
# Не линкуйте весь AdvancedServer внутрь DLL без необходимости.
# Экспортируйте plugin_entry.
```

Скопируйте артефакт в `plugins/MyPlugin.dll` (или `.so`) **рядом с cwd сервера** и запустите сервер, собранный с `-DPLUGIN_DYNAMIC_LOADING=ON`.

Убедитесь, что `api_version` совпадает с хостом.

---

## 11. ExtendedCore как эталон

Включён по умолчанию (`WITH_EXTENDED_CORE=ON`). Реализует:

- аккаунты / `.login` / `.register`
- меню / shop / inventory
- skill rating на results
- abilities + anticheat через хуки v2
- promo-подсказки

Исходники: только [`plugins/ExtendedCore/`](./ExtendedCore/) — без форков `Lobby.c` / `Game.c` в этом каталоге.

Отключение:

```bash
cmake .. -DWITH_EXTENDED_CORE=OFF
```

Получите stock AdvancedServer с рабочим plugin API, но без логики ExtendedCore.

---

## 12. Чеклист перед релизом плагина

- [ ] `api_version == PLUGIN_API_VERSION`
- [ ] Уникальное `name`
- [ ] `peer_data_size` / `server_data_size` достаточны; нет записи мимо буфера
- [ ] Хуки не делают долгий I/O без нужды
- [ ] `PLUGIN_HANDLED` используется осознанно (чат/пакеты/смерть)
- [ ] Проверена сборка с и без вашего плагина
- [ ] При динамической загрузке проверен экспорт `plugin_entry`
- [ ] При старте имя плагина видно в блоке `--------- Plugins (N) ---------`

---

## 13. Куда смотреть в коде

| Файл | Роль |
|------|------|
| `include/Plugin.h` | Публичный API |
| `advancedserver/Plugin.c` | Реестр, userdata, динамическая загрузка, логирование списка |
| `advancedserver/Server.c` / `Lobby.c` / `Game.c` / `Lib.c` | Точки вызова хуков |
| `plugins/ExtendedCore/` | Полный пример |

Вопросы по расширению API (новые хуки) — через изменения ядра в `Plugin.h` + вызовы в stock-коде; плагины после этого пересобираются под новый `PLUGIN_API_VERSION`.
