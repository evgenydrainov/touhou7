#include "Stage.h"

#include "Game.h"
#include "GameScene.h"

#include "common.h"
#include "cpml.h"

#include <iostream>

#define BOSS_STARTING_X ((float)PLAY_AREA_W / 2.0f)
#define BOSS_STARTING_Y 96.0f

#define PLAYER_STARTING_X ((float)PLAY_AREA_W / 2.0f)
#define PLAYER_STARTING_Y 384.0f

namespace th
{
	enum ObjectType : unsigned char {
		TYPE_BULLET,
		TYPE_ENEMY,
		TYPE_PLAYER,
		TYPE_BOSS
	};

	static void lua_checkargc(lua_State* L, int min, int max) {
		int top = lua_gettop(L);
		if (top < min || top > max) {
			if (min == max) {
				luaL_error(L, "expected %d args got %d", min, top);
			} else if (top < min) {
				luaL_error(L, "too little args (at least %d)", min);
			} else {
				luaL_error(L, "too much args (max %d)", max);
			}
		}
	}

	static Game* lua_getcontext(lua_State* L) {
		lua_rawgetp(L, LUA_REGISTRYINDEX, nullptr);
		Game* ctx = (Game*)lua_touserdata(L, -1);
		lua_pop(L, 1);
		return ctx;
	}

	static int lua_random(lua_State* L) {
		lua_checkargc(L, 0, 2);

		float a = 0.0f;
		float b = 1.0f;

		int argc = lua_gettop(L);
		if (argc > 0) {
			b = (float)luaL_checknumber(L, 1);
			if (argc > 1) {
				a = b;
				b = (float)luaL_checknumber(L, 2);
			}
		}

		Game* ctx = lua_getcontext(L);
		float r = ctx->scene.stage.random_range(a, b);
		lua_pushnumber(L, (lua_Number) r);

		return 1;
	}

	static float lua_named_argf(lua_State* L, int argc, int i, const char* name, float def = 0.0f) {
		float res = def;
		if (argc >= i) {
			lua_geti(L, 1, i);
			res = (float)luaL_checknumber(L, -1);
			lua_pop(L, 1);
		} else {
			lua_getfield(L, 1, name);
			if (!lua_isnil(L, -1)) {
				res = (float)luaL_checknumber(L, -1);
			}
			lua_pop(L, 1);
		}
		return res;
	}

	static int lua_named_argi(lua_State* L, int argc, int i, const char* name, int def = 0) {
		int res = def;
		if (argc >= i) {
			lua_geti(L, 1, i);
			res = (int)luaL_checkinteger(L, -1);
			lua_pop(L, 1);
		} else {
			lua_getfield(L, 1, name);
			if (!lua_isnil(L, -1)) {
				res = (int)luaL_checkinteger(L, -1);
			}
			lua_pop(L, 1);
		}
		return res;
	}

	static const char* lua_named_argstr(lua_State* L, int argc, int i, const char* name, const char* def = nullptr) {
		const char* res = def;
		if (argc >= i) {
			lua_geti(L, 1, i);
			res = luaL_checkstring(L, -1);
			lua_pop(L, 1);
		} else {
			lua_getfield(L, 1, name);
			if (!lua_isnil(L, -1)) {
				res = luaL_checkstring(L, -1);
			}
			lua_pop(L, 1);
		}
		return res;
	}

	static bool lua_named_argfunc(lua_State* L, int argc, int i, const char* name) {
		bool res = false;
		if (argc >= i) {
			lua_geti(L, 1, i);
			luaL_checktype(L, -1, LUA_TFUNCTION);
			res = true;
		} else {
			lua_getfield(L, 1, name);
			if (!lua_isnil(L, -1)) {
				luaL_checktype(L, -1, LUA_TFUNCTION);
				res = true;
			} else {
				lua_pop(L, 1);
			}
		}
		return res;
	}

	static int CreateCoroutine(lua_State* L, lua_State* main_L) {
		if (lua_gettop(L) < 1) return LUA_REFNIL;

		if (!lua_isfunction(L, -1)) {
			lua_pop(L, 1);
			return LUA_REFNIL;
		}

		lua_State* NL = lua_newthread(main_L);
		int coroutine = luaL_ref(main_L, LUA_REGISTRYINDEX);
		lua_xmove(L, NL, 1);
		return coroutine;
	}

	static int lua_getargc(lua_State* L) {
		lua_checkargc(L, 1, 1);
		luaL_checktype(L, 1, LUA_TTABLE);

		lua_len(L, 1);
		int argc = (int)lua_tointeger(L, -1);
		lua_pop(L, 1);

		return argc;
	}

	static int lua_CreateEnemy(lua_State* L) {
		int argc = lua_getargc(L);

		float x   = lua_named_argf(L, argc, 1, "x", PLAY_AREA_W / 2.0f);
		float y   = lua_named_argf(L, argc, 2, "y", 0.0f);
		float spd = lua_named_argf(L, argc, 3, "spd");
		float dir = lua_named_argf(L, argc, 4, "dir");
		float acc = lua_named_argf(L, argc, 5, "acc");
		const char* spr = lua_named_argstr(L, argc, 6, "spr", "");
		int drops = lua_named_argi(L, argc, 7, "drops");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 8, "Script")) {
			coroutine = CreateCoroutine(L, ctx->scene.stage.L); // it's possible to get the main thread without context
		}

		int death_callback = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 9, "OnDeath")) {
			death_callback = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		int update_callback = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 10, "OnUpdate")) {
			update_callback = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		Enemy& enemy = ctx->scene.stage.CreateEnemy();
		enemy.x = x;
		enemy.y = y;
		enemy.spd = spd;
		enemy.dir = dir;
		enemy.acc = acc;
		enemy.sc.sprite = ctx->assets.GetSprite(spr);
		enemy.drops = drops;
		enemy.coroutine = coroutine;
		enemy.death_callback = death_callback;
		enemy.update_callback = update_callback;
		lua_pushinteger(L, enemy.id);
		return 1;
	}

	static int lua_CreateBoss(lua_State* L) {
		int argc = lua_getargc(L);

		float x   = lua_named_argf(L, argc, 1, "x", BOSS_STARTING_X);
		float y   = lua_named_argf(L, argc, 2, "y", BOSS_STARTING_Y);
		float spd = lua_named_argf(L, argc, 3, "spd");
		float dir = lua_named_argf(L, argc, 4, "dir");
		float acc = lua_named_argf(L, argc, 5, "acc");
		int type = lua_named_argi(L, argc, 6, "type");

		Game* ctx = lua_getcontext(L);

		Boss& boss = ctx->scene.stage.CreateBoss();
		boss.x = x;
		boss.y = y;
		boss.spd = spd;
		boss.dir = dir;
		boss.acc = acc;
		boss.radius = 25.0f;
		boss.sc.sprite = stage_data[0].boss_data[type].sprite;
		boss.type_index = type;

		ctx->scene.stage.BossStartPhase(boss);

		lua_pushinteger(L, boss.id);
		return 1;
	}

	static int lua_Shoot(lua_State* L) {
		int argc = lua_getargc(L);

		float x   = lua_named_argf(L, argc, 1, "x");
		float y   = lua_named_argf(L, argc, 2, "y");
		float spd = lua_named_argf(L, argc, 3, "spd");
		float dir = lua_named_argf(L, argc, 4, "dir");
		float acc = lua_named_argf(L, argc, 5, "acc");
		int type = lua_named_argi(L, argc, 6, "type");
		int color = lua_named_argi(L, argc, 7, "color");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 8, "Script")) {
			coroutine = CreateCoroutine(L, ctx->scene.stage.L); // main thread without context
		}

		Bullet& bullet = ctx->scene.stage.CreateBullet();
		bullet.x = x;
		bullet.y = y;
		bullet.spd = spd;
		bullet.dir = dir;
		bullet.acc = acc;
		bullet.radius = bullet_data[type].radius;
		bullet.sc.sprite = bullet_data[type].sprite;
		bullet.sc.frame_index = (float)color;
		bullet.rotate = bullet_data[type].rotate;
		bullet.coroutine = coroutine;
		lua_pushinteger(L, bullet.id);
		return 1;
	}

	template <
		typename T,
		void (*PushFunc)(lua_State* L, T value),
		T (*GetFromBullet)(Bullet* bullet),
		T (*GetFromEnemy)(Enemy* enemy),
		T (*GetFromPlayer)(Player* player),
		T (*GetFromBoss)(Boss* boss),
		T (*GetFromStage)() = nullptr
	>
	static int lua_GetObjectVar(lua_State* L) {
		lua_checkargc(L, 1, 1);
		unsigned int id = (unsigned int)luaL_checkinteger(L, 1);
		unsigned char type = id >> 28;

		Game* ctx = lua_getcontext(L);
		T value{};
		switch (type) {
			case TYPE_BULLET: {
				if (Bullet* bullet = ctx->scene.stage.FindBullet(id)) {
					value = GetFromBullet(bullet);
				}
				break;
			}
			case TYPE_ENEMY: {
				if (Enemy* enemy = ctx->scene.stage.FindEnemy(id)) {
					value = GetFromEnemy(enemy);
				}
				break;
			}
			case TYPE_PLAYER: {
				if (Player* player = ctx->scene.stage.FindPlayer(id)) {
					value = GetFromPlayer(player);
				}
				break;
			}
			case TYPE_BOSS: {
				if (Boss* boss = ctx->scene.stage.FindBoss(id)) {
					value = GetFromBoss(boss);
				}
				break;
			}
			case 0xf: {
				if (GetFromStage != nullptr) value = GetFromStage();
				break;
			}
		}
		PushFunc(L, value);
		return 1;
	}

	template <
		typename T,
		T (*ToFunc)(lua_State* L, int idx),
		void (*BulletSet)(Bullet* bullet, T value),
		void (*EnemySet)(Enemy* enemy, T value),
		void (*PlayerSet)(Player* player, T value),
		void (*BossSet)(Boss* boss, T value)
	>
	static int lua_SetObjectVar(lua_State* L) {
		lua_checkargc(L, 2, 2);
		unsigned int id = (unsigned int)luaL_checkinteger(L, 1);
		T value = ToFunc(L, 2);
		unsigned char type = id >> 28;

		Game* ctx = lua_getcontext(L);
		switch (type) {
			case TYPE_BULLET: {
				if (Bullet* bullet = ctx->scene.stage.FindBullet(id)) {
					BulletSet(bullet, value);
				}
				break;
			}
			case TYPE_ENEMY: {
				if (Enemy* enemy = ctx->scene.stage.FindEnemy(id)) {
					EnemySet(enemy, value);
				}
				break;
			}
			case TYPE_PLAYER: {
				if (Player* player = ctx->scene.stage.FindPlayer(id)) {
					PlayerSet(player, value);
				}
				break;
			}
			case TYPE_BOSS: {
				if (Boss* boss = ctx->scene.stage.FindBoss(id)) {
					BossSet(boss, value);
				}
				break;
			}
		}
		return 0;
	}

	static int lua_Exists(lua_State* L) {
		lua_checkargc(L, 1, 1);
		unsigned int id = (unsigned int)luaL_checkinteger(L, 1);
		unsigned char type = id >> 28;

		Game* ctx = lua_getcontext(L);
		bool result = false;
		switch (type) {
			case TYPE_BULLET: {
				result = ctx->scene.stage.FindBullet(id);
				break;
			}
			case TYPE_ENEMY: {
				result = ctx->scene.stage.FindEnemy(id);
				break;
			}
			case TYPE_PLAYER: {
				result = ctx->scene.stage.FindPlayer(id);
				break;
			}
			case TYPE_BOSS: {
				result = ctx->scene.stage.FindBoss(id);
				break;
			}
		}
		lua_pushboolean(L, result);
		return 1;
	}

	static void _lua_register(lua_State* L, const char* name, lua_CFunction func) {
		lua_register(L, name, func);
	}

	static void ObjectVarPushFloat(lua_State* L, float value) { lua_pushnumber(L, value); }
	static void ObjectVarPushID(lua_State* L, unsigned int value) { lua_pushinteger(L, value); }

	static float ObjectVarToFloat(lua_State* L, int idx) { return (float)luaL_checknumber(L, idx); }
	static unsigned int ObjectVarToID(lua_State* L, int idx) { return (unsigned int)luaL_checkinteger(L, idx); }

	static float GetXFromBullet(Bullet* bullet) { return bullet->x; }
	static float GetXFromEnemy(Enemy* enemy) { return enemy->x; }
	static float GetXFromPlayer(Player* player) { return player->x; }
	static float GetXFromBoss(Boss* boss) { return boss->x; }

	static float GetYFromBullet(Bullet* bullet) { return bullet->y; }
	static float GetYFromEnemy(Enemy* enemy) { return enemy->y; }
	static float GetYFromPlayer(Player* player) { return player->y; }
	static float GetYFromBoss(Boss* boss) { return boss->y; }

	static float GetSpdFromBullet(Bullet* bullet) { return bullet->spd; }
	static float GetSpdFromEnemy(Enemy* enemy) { return enemy->spd; }
	static float GetSpdFromPlayer(Player* player) { return cpml::point_distance(0.0f, 0.0f, player->hsp, player->vsp); }
	static float GetSpdFromBoss(Boss* boss) { return boss->spd; }

	static float GetDirFromBullet(Bullet* bullet) { return bullet->dir; }
	static float GetDirFromEnemy(Enemy* enemy) { return enemy->dir; }
	static float GetDirFromPlayer(Player* player) { return cpml::point_direction(0.0f, 0.0f, player->hsp, player->vsp); }
	static float GetDirFromBoss(Boss* boss) { return boss->dir; }

	static float GetAccFromBullet(Bullet* bullet) { return bullet->acc; }
	static float GetAccFromEnemy(Enemy* enemy) { return enemy->acc; }
	static float GetAccFromPlayer(Player* player) { return 0.0f; }
	static float GetAccFromBoss(Boss* boss) { return boss->acc; }

	static unsigned int GetTargetFromBullet(Bullet* bullet) { return 0 | (TYPE_PLAYER << 28); }
	static unsigned int GetTargetFromEnemy(Enemy* enemy) { return 0 | (TYPE_PLAYER << 28); }
	static unsigned int GetTargetFromPlayer(Player* player) { return 0 | (TYPE_PLAYER << 28); }
	static unsigned int GetTargetFromBoss(Boss* boss) { return 0 | (TYPE_PLAYER << 28); }
	static unsigned int GetTargetFromStage() { return 0 | (TYPE_PLAYER << 28); }

	static void SetXForBullet(Bullet* bullet, float value) { bullet->x = value; }
	static void SetXForEnemy(Enemy* enemy, float value) { enemy->x = value; }
	static void SetXForPlayer(Player* player, float value) { player->x = value; }
	static void SetXForBoss(Boss* boss, float value) { boss->x = value; }

	static void SetYForBullet(Bullet* bullet, float value) { bullet->y = value; }
	static void SetYForEnemy(Enemy* enemy, float value) { enemy->y = value; }
	static void SetYForPlayer(Player* player, float value) { player->y = value; }
	static void SetYForBoss(Boss* boss, float value) { boss->y = value; }

	static void SetSpdForBullet(Bullet* bullet, float value) { bullet->spd = value; }
	static void SetSpdForEnemy(Enemy* enemy, float value) { enemy->spd = value; }
	static void SetSpdForPlayer(Player* player, float value) {}
	static void SetSpdForBoss(Boss* boss, float value) { boss->spd = value; }

	static void SetDirForBullet(Bullet* bullet, float value) { bullet->dir = value; }
	static void SetDirForEnemy(Enemy* enemy, float value) { enemy->dir = value; }
	static void SetDirForPlayer(Player* player, float value) {}
	static void SetDirForBoss(Boss* boss, float value) { boss->dir = value; }

	static void SetAccForBullet(Bullet* bullet, float value) { bullet->acc = value; }
	static void SetAccForEnemy(Enemy* enemy, float value) { enemy->acc = value; }
	static void SetAccForPlayer(Player* player, float value) {}
	static void SetAccForBoss(Boss* boss, float value) { boss->acc = value; }

	bool Stage::Init()
	{
		if (!(L = luaL_newstate())) {
			TH_SHOW_ERROR("luaL_newstate failed");
		}

		{
			lua_pushlightuserdata(L, &game);
			lua_rawsetp(L, LUA_REGISTRYINDEX, nullptr);
		}

		{
			const luaL_Reg loadedlibs[] = {
				{LUA_GNAME, luaopen_base},
				{LUA_COLIBNAME, luaopen_coroutine},
				{LUA_TABLIBNAME, luaopen_table},
				{LUA_STRLIBNAME, luaopen_string},
				{LUA_MATHLIBNAME, luaopen_math},
				{LUA_UTF8LIBNAME, luaopen_utf8},
				{NULL, NULL}
			};

			const luaL_Reg *lib;
			/* "require" functions from 'loadedlibs' and set results to global table */
			for (lib = loadedlibs; lib->func; lib++) {
				luaL_requiref(L, lib->name, lib->func, 1);
				lua_pop(L, 1);  /* remove lib */
			}
		}

		{
			lua_register(L, "random", lua_random);
			lua_register(L, "CreateEnemy", lua_CreateEnemy);
			lua_register(L, "CreateBoss", lua_CreateBoss);
			lua_register(L, "Shoot", lua_Shoot);
			lua_register(L, "Exists", lua_Exists);

			lua_CFunction GetX = lua_GetObjectVar<float, ObjectVarPushFloat, GetXFromBullet, GetXFromEnemy, GetXFromPlayer, GetXFromBoss>;
			lua_CFunction GetY = lua_GetObjectVar<float, ObjectVarPushFloat, GetYFromBullet, GetYFromEnemy, GetYFromPlayer, GetYFromBoss>;
			lua_CFunction GetSpd = lua_GetObjectVar<float, ObjectVarPushFloat, GetSpdFromBullet, GetSpdFromEnemy, GetSpdFromPlayer, GetSpdFromBoss>;
			lua_CFunction GetDir = lua_GetObjectVar<float, ObjectVarPushFloat, GetDirFromBullet, GetDirFromEnemy, GetDirFromPlayer, GetDirFromBoss>;
			lua_CFunction GetAcc = lua_GetObjectVar<float, ObjectVarPushFloat, GetAccFromBullet, GetAccFromEnemy, GetAccFromPlayer, GetAccFromBoss>;
			lua_CFunction GetTarget = lua_GetObjectVar<unsigned int, ObjectVarPushID, GetTargetFromBullet, GetTargetFromEnemy, GetTargetFromPlayer, GetTargetFromBoss, GetTargetFromStage>;

			_lua_register(L, "GetX", GetX);
			_lua_register(L, "GetY", GetY);
			_lua_register(L, "GetSpd", GetSpd);
			_lua_register(L, "GetDir", GetDir);
			_lua_register(L, "GetAcc", GetAcc);
			_lua_register(L, "GetTarget", GetTarget);

			lua_CFunction SetX = lua_SetObjectVar<float, ObjectVarToFloat, SetXForBullet, SetXForEnemy, SetXForPlayer, SetXForBoss>;
			lua_CFunction SetY = lua_SetObjectVar<float, ObjectVarToFloat, SetYForBullet, SetYForEnemy, SetYForPlayer, SetYForBoss>;
			lua_CFunction SetSpd = lua_SetObjectVar<float, ObjectVarToFloat, SetSpdForBullet, SetSpdForEnemy, SetSpdForPlayer, SetSpdForBoss>;
			lua_CFunction SetDir = lua_SetObjectVar<float, ObjectVarToFloat, SetDirForBullet, SetDirForEnemy, SetDirForPlayer, SetDirForBoss>;
			lua_CFunction SetAcc = lua_SetObjectVar<float, ObjectVarToFloat, SetAccForBullet, SetAccForEnemy, SetAccForPlayer, SetAccForBoss>;

			_lua_register(L, "SetX", SetX);
			_lua_register(L, "SetY", SetY);
			_lua_register(L, "SetSpd", SetSpd);
			_lua_register(L, "SetDir", SetDir);
			_lua_register(L, "SetAcc", SetAcc);
		}

		const auto& scripts = game.assets.GetScripts();

		for (auto script = scripts.begin(); script != scripts.end(); ++script) {
			const char* buffer = script->second.data();
			size_t buffer_size = script->second.size();
			const char* debug_name = script->first.c_str();

			if (luaL_loadbuffer(L, buffer, buffer_size, debug_name) != LUA_OK) {
				TH_SHOW_ERROR("luaL_loadbuffer failed\n%s", lua_tostring(L, -1));
				lua_settop(L, 0);
				continue;
			}

			if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
				TH_SHOW_ERROR("error while running script\n%s", lua_tostring(L, -1));
				lua_settop(L, 0);
				continue;
			}
		}

		{
			lua_getglobal(L, stage_data[0].script);
			coroutine = CreateCoroutine(L, L);
		}

		CreatePlayer();

		return true;
	}

	void Stage::Quit()
	{
		lua_close(L); // crashes if L is null
	}

#define PLAYER_DEATH_TIME      20.0f
#define PLAYER_APPEAR_TIME     20.0f
#define PLAYER_RESPAWN_IFRAMES 120.0f
#define PLAYER_BOMB_TIME       (2.5f * 60.0f)

	void Stage::Update(float delta)
	{
		// update
		{
			// update player
			{
				Player& player = player_storage;

				switch (player.state) {
					case PlayerState::Normal: {
						const Uint8* key = SDL_GetKeyboardState(nullptr);

						player.focus = key[SDL_SCANCODE_LSHIFT];

						float xmove = 0.0f;
						float ymove = 0.0f;

						if (key[SDL_SCANCODE_LEFT]) xmove -= 1.0f;
						if (key[SDL_SCANCODE_RIGHT]) xmove += 1.0f;
						if (key[SDL_SCANCODE_UP]) ymove -= 1.0f;
						if (key[SDL_SCANCODE_DOWN]) ymove += 1.0f;

						float len = cpml::point_distance(0.0f, 0.0f, xmove, ymove);
						if (len != 0.0f) {
							xmove /= len;
							ymove /= len;
						}

						float spd = player.focus ? char_data[game.player_character].focus_spd : char_data[game.player_character].move_spd;
						player.hsp = xmove * spd;
						player.vsp = ymove * spd;

						if (char_data[game.player_character].shot_type) {
							(*char_data[game.player_character].shot_type)(&game, delta);
						}

						if (key[SDL_SCANCODE_X]) {
							if (player.bomb_timer == 0.f) {
								if (scene.stats.bombs > 0) {
									if (char_data[game.player_character].bomb) {
										(*char_data[game.player_character].bomb)(&game);
									}
									scene.stats.bombs--;
									player.bomb_timer = PLAYER_BOMB_TIME;
								}
							}
						}

						player.iframes = std::max(player.iframes - delta, 0.0f);

						if (xmove != 0.0f) {
							if (player.sc.sprite != char_data[game.player_character].turn_spr) {
								player.sc.sprite = char_data[game.player_character].turn_spr;
								player.sc.frame_index = 0.f;
							}
							if (xmove > 0.0f) {
								player.sc.xscale = -1.f;
							} else {
								player.sc.xscale = 1.f;
							}
						}

						if (xmove == 0.0f && player.sc.sprite == char_data[game.player_character].turn_spr) {
							if (player.sc.frame_index > (float)player.sc.sprite->loop_frame - 1.0f) {
								player.sc.frame_index = (float)player.sc.sprite->loop_frame - 1.0f;
							}
							player.sc.frame_index -= player.sc.sprite->anim_spd * delta;
							if (player.sc.frame_index < 0.0f) {
								player.sc.sprite = char_data[game.player_character].idle_spr;
								player.sc.frame_index = 0.0f;
							}
						} else {
							player.sc.frame_index += player.sc.sprite->anim_spd * delta;
							if ((int)player.sc.frame_index >= player.sc.sprite->frame_count) {
								player.sc.frame_index = (float)player.sc.sprite->loop_frame + fmodf(player.sc.frame_index - (float)player.sc.sprite->loop_frame, (float)(player.sc.sprite->frame_count - player.sc.sprite->loop_frame));
							}
						}
						break;
					}
					case PlayerState::Dying: {

						break;
					}
					case PlayerState::Appearing: {

						break;
					}
				}

				player.bomb_timer = std::max(player.bomb_timer - delta, 0.0f);

				if (player.focus) {
					player.hitbox_alpha = cpml::approach(player.hitbox_alpha, 1.0f, 0.1f * delta);
				} else {
					player.hitbox_alpha = cpml::approach(player.hitbox_alpha, 0.0f, 0.1f * delta);
				}
			}

			// update boss
			{
				if (boss_exists) {
					Boss& boss = boss_storage;
					BossUpdate(boss, delta);
				}
			}
		}

		{
			float physics_update_rate = 1.0f / 300.0f; // 300 fps
			float physics_timer = delta * 1.0f;//g_stage->gameplay_delta;
			while (physics_timer > 0.0f) {
				float pdelta = std::min(physics_timer, physics_update_rate * 60.0f);
				PhysicsUpdate(pdelta);
				physics_timer -= pdelta;
			}
		}

		{
			coro_update_timer += delta;
			while (coro_update_timer >= 1.0f) {
				CallCoroutines();
				coro_update_timer -= 1.0f;
			}
		}

		{
			{
				Player& player = player_storage;
				player.x = std::clamp(player.x, 0.0f, (float)PLAY_AREA_W - 1.0f);
				player.y = std::clamp(player.y, 0.0f, (float)PLAY_AREA_H - 1.0f);
			}

			for (auto bullet = bullets.begin(); bullet != bullets.end();) {
				float x = bullet->x;
				float y = bullet->y;
				if (x < 0.0f || x >= (float)PLAY_AREA_W || y < 0.0f || y >= (float)PLAY_AREA_H) {
					FreeBullet(*bullet);
					bullet = bullets.erase(bullet);
					continue;
				}
				bullet->lifetime += delta;
				if (bullet->lifetime > 60.0f * 60.0f) {
					FreeBullet(*bullet);
					bullet = bullets.erase(bullet);
					continue;
				}
				++bullet;
			}
		}

		{
			{
				Player& player = player_storage;
				//UpdateSpriteComponent(player.sc, delta);
			}

			if (boss_exists) {
				Boss& boss = boss_storage;
				UpdateSpriteComponent(boss.sc, delta);
			}

			for (Enemy& enemy : enemies) {
				UpdateSpriteComponent(enemy.sc, delta);
			}

			for (Bullet& bullet : bullets) {
				UpdateSpriteComponent(bullet.sc, delta);
				if (bullet.rotate) {
					bullet.sc.angle = bullet.dir - 90.0f;
				}
			}
		}

		{
			//Player& player = player_storage;
			//if (player.dead) {
			//	FreePlayer(player);
			//}
			if (boss_exists) {
				Boss& boss = boss_storage;
				if (boss.dead) {
					FreeBoss(boss);
				}
			}
			for (auto bullet = bullets.begin(); bullet != bullets.end();) {
				if (bullet->dead) {
					FreeBullet(*bullet);
					bullet = bullets.erase(bullet);
					continue;
				}
				++bullet;
			}
			for (auto enemy = enemies.begin(); enemy != enemies.end();) {
				if (enemy->dead) {
					FreeEnemy(*enemy);
					enemy = enemies.erase(enemy);
					continue;
				}
				++enemy;
			}
		}

		time += delta;
	}

	void Stage::UpdateSpriteComponent(SpriteComponent& sc, float delta)
	{
		SpriteData* sprite = sc.sprite;
		int frame_count = sprite->frame_count;
		int loop_frame = sprite->loop_frame;
		float anim_spd = sprite->anim_spd;

		sc.frame_index += anim_spd * delta;
		if ((int)sc.frame_index >= frame_count) {
			sc.frame_index = (float)loop_frame + fmodf(sc.frame_index - (float)loop_frame, (float)(frame_count - loop_frame));
		}
	}

	template <typename T>
	static void MoveObject(T& object, float delta) {
		object.x += cpml::lengthdir_x(object.spd, object.dir) * delta;
		object.y += cpml::lengthdir_y(object.spd, object.dir) * delta;
		object.spd += object.acc * delta;

		if (object.spd < 0.0f) {
			object.spd = 0.0f;
		}
	}

	void Stage::PhysicsUpdate(float delta)
	{
		{
			{
				Player& player = player_storage;
				player.x += player.hsp * delta;
				player.y += player.vsp * delta;
			}

			if (boss_exists) {
				Boss& boss = boss_storage;
				MoveObject(boss, delta);
			}

			for (Enemy& enemy : enemies) {
				MoveObject(enemy, delta);
			}

			for (Bullet& bullet : bullets) {
				MoveObject(bullet, delta);
			}
		}

		{
		
		}
	}

	#define BOSS_PHASE_START_TIME 60.0f
	#define BOSS_SPELL_END_TIME   60.0f

	template <typename Object>
	static void LaunchTowardsPoint(Object& object, float target_x, float target_y, float acc) {
		acc = fabsf(acc);
		float dist = cpml::point_distance(object.x, object.y, target_x, target_y);
		object.spd = sqrtf(dist * acc * 2.0f);
		object.acc = -acc;
		object.dir = cpml::point_direction(object.x, object.y, target_x, target_y);
	}

	void Stage::BossStartPhase(Boss& boss)
	{
		boss.hp = stage_data[0].boss_data[boss.type_index].phase_data[boss.phase_index].hp;
		boss.timer = stage_data[0].boss_data[boss.type_index].phase_data[boss.phase_index].time;
		boss.wait_timer = BOSS_PHASE_START_TIME;
		boss.state = BossState::WaitingStart;

		if (boss.phase_index > 0) {
			LaunchTowardsPoint(boss, BOSS_STARTING_X, BOSS_STARTING_Y, 0.05);
		}
	}

	bool Stage::BossEndPhase(Boss& boss) {
		for (Bullet& bullet : bullets) {
			// drop a pickup
		}
		bullets.clear();

		if (boss.coroutine != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, boss.coroutine);
			boss.coroutine = LUA_REFNIL;
		}

		if (boss.phase_index + 1 < stage_data[0].boss_data[boss.type_index].phase_count) {
			if (stage_data[0].boss_data[boss.type_index].phase_data[boss.phase_index].type = PHASE_SPELLCARD) {
				//for (int i = 0; i < 5; i++) {
				//	auto& pos = g_reg->get<Position>(e);
				//	float x = pos.x + g_stage->random_range(-50.f, 50.f);
				//	float y = pos.y + g_stage->random_range(-50.f, 50.f);
				//	int type = (i == 4) ? PICKUP_BIGP : PICKUP_POWER;
				//	drop_pickup(x, y, type);
				//}

				boss.state = BossState::WaitingEnd;
				boss.wait_timer = BOSS_SPELL_END_TIME;
			} else {
				boss.phase_index++;
				BossStartPhase(boss);
			}
		} else {
			//return false;
			boss.dead = true;
		}

		return true;
	}

	void Stage::BossUpdate(Boss& boss, float delta) {
		switch (boss.state) {
			case BossState::Normal: {
				boss.timer = std::max(boss.timer - delta, 0.0f);
				if (boss.timer == 0.f) {
					BossEndPhase(boss);
				}
				break;
			}
			case BossState::WaitingStart: {
				boss.wait_timer = std::max(boss.wait_timer - delta, 0.0f);
				if (boss.wait_timer == 0.f) {
					boss.state = BossState::Normal;
					lua_getglobal(L, stage_data[0].boss_data[boss.type_index].phase_data[boss.phase_index].script);
					boss.coroutine = CreateCoroutine(L, L);
				}
				break;
			}
			case BossState::WaitingEnd: {
				boss.wait_timer = std::max(boss.wait_timer - delta, 0.0f);
				if (boss.wait_timer == 0.0f) {
					boss.phase_index++;
					BossStartPhase(boss);
				}
				break;
			}
		}

		if (boss.spd > 0.1f) {
			if (90.0f <= boss.dir && boss.dir < 270.0f) {
				boss.sc.xscale = -1.f;
			} else {
				boss.sc.xscale = 1.f;
			}
		}

		//std::cout << "hp " << boss.hp << std::endl;
		//std::cout << "timer " << boss.timer << std::endl;
		//std::cout << "wait timer " << boss.wait_timer << std::endl;
		//std::cout << "phase " << boss.phase_index << std::endl;
	}

	static void UpdateCoroutine(lua_State* L, int* coroutine, unsigned int id)
	{
		if (*coroutine == LUA_REFNIL) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, *coroutine);
		if (!lua_isthread(L, -1)) {
			TH_SHOW_ERROR("coroutine ref is not a thread");
			lua_settop(L, 0);
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
			return;
		}

		lua_State* NL = lua_tothread(L, -1);
		lua_pop(L, 1);

		if (!lua_isyieldable(NL)) {
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
			return;
		}

		lua_pushinteger(NL, id);
		int nres;
		int res = lua_resume(NL, L, 1, &nres);
		if (res == LUA_OK) {
			lua_pop(NL, nres);
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
		} else if (res != LUA_YIELD) {
			TH_SHOW_ERROR("error while running stage coroutine\n%s", lua_tostring(NL, -1));
			lua_settop(NL, 0);
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
		}
	}

	void Stage::CallCoroutines()
	{
		UpdateCoroutine(L, &coroutine, -1);

		if (boss_exists) {
			Boss& boss = boss_storage;
			UpdateCoroutine(L, &boss.coroutine, 0 | (TYPE_BOSS << 28));
		}

		for (Enemy& enemy : enemies) {
			UpdateCoroutine(L, &enemy.coroutine, enemy.id);
		}

		for (Bullet& bullet : bullets) {
			UpdateCoroutine(L, &bullet.coroutine, bullet.id);
		}
	}

	template <typename Object>
	static void DrawObject(SDL_Renderer* renderer, const Object& object) {
		float x = object.x;
		float y = object.y;
		SpriteData* sprite = object.sc.sprite;
		int frame_index = (int)object.sc.frame_index;
		float angle = object.sc.angle;
		DrawSprite(renderer, sprite, frame_index, x, y, angle, object.sc.xscale, object.sc.yscale);
	}

	void Stage::Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta)
	{
		SDL_SetRenderTarget(renderer, target);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			for (const Enemy& enemy : enemies) {
				DrawObject(renderer, enemy);
			}

			if (boss_exists) {
				Boss& boss = boss_storage;
				DrawObject(renderer, boss);
			}

			{
				Player& player = player_storage;
				DrawObject(renderer, player);
				if (player.hitbox_alpha > 0.0f) {
					SpriteData* sprite = game.assets.GetSprite("Hitbox");
					unsigned char a = (unsigned char) (255.0f * player.hitbox_alpha);
					DrawSprite(renderer, sprite, 0, player.x, player.y, -time, 1.0f, 1.0f, 255, 255, 255, a);
				}
			}

			// pickups

			// player bullets

			for (const Bullet& bullet : bullets) {
				DrawObject(renderer, bullet);
			}


			// GUI
			{
				if (boss_exists) {
					Boss& boss = boss_storage;

					// phases left
					{
						char buf[3];
						snprintf(buf, 3, "%d", stage_data[0].boss_data[boss.type_index].phase_count - boss.phase_index - 1);
						int x = 0;
						int y = 0;
						DrawText(renderer, &game.assets.fntMain, buf, x, y);
					}

					// healthbar
					{
						int healthbar_x = 32 + 2;
						int healthbar_y = 6;
						int healthbar_w = PLAY_AREA_W - 64 - 4;
						int healthbar_h = 2;
						int reduced_w = (int) ((float)healthbar_w * (boss.hp / stage_data[0].boss_data[boss.type_index].phase_data[boss.phase_index].hp));
						{
							SDL_Rect rect{healthbar_x, healthbar_y + 1, reduced_w, healthbar_h};
							SDL_SetRenderDrawColor(renderer, 130, 130, 130, 255);
							SDL_RenderFillRect(renderer, &rect);
						}
						{
							SDL_Rect rect{healthbar_x, healthbar_y, reduced_w, healthbar_h};
							SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
							SDL_RenderFillRect(renderer, &rect);
						}
					}

					// timer
					{
						char buf[4];
						snprintf(buf, 4, "%3d", (int)(boss.timer / 60.0f));
						int x = PLAY_AREA_W - 3 * 15;
						int y = 0;
						DrawText(renderer, &game.assets.fntMain, buf, x, y);
					}
				}
			}
		}
		SDL_SetRenderTarget(renderer, nullptr);
	}

	Enemy& Stage::CreateEnemy()
	{
		Enemy& enemy = enemies.emplace_back();
		enemy.id = (next_id++) | (TYPE_ENEMY << TYPE_PART_SHIFT);
		return enemy;
	}

	Bullet& Stage::CreateBullet()
	{
		Bullet& bullet = bullets.emplace_back();
		bullet.id = (next_id++) | (TYPE_BULLET << TYPE_PART_SHIFT);
		return bullet;
	}

	Player& Stage::CreatePlayer()
	{
		Player& player = (player_storage = {});
		player.id = 0 | (TYPE_PLAYER << TYPE_PART_SHIFT);
		player.x = PLAYER_STARTING_X;
		player.y = PLAYER_STARTING_Y;
		player.sc.sprite = game.assets.GetSprite("ReimuIdle");
		return player;
	}

	Boss& Stage::CreateBoss()
	{
		Boss& boss = (boss_storage = {});
		boss.id = 0 | (TYPE_BOSS << TYPE_PART_SHIFT);
		boss_exists = true;
		return boss;
	}

	typedef ptrdiff_t ssize;

	template <typename T>
	static T* BinarySearch(std::vector<T>& storage, unsigned int id)
	{
		ssize left = 0;
		ssize right = (ssize)storage.size() - 1;

		while (left <= right) {
			ssize middle = (left + right) / 2;
			if (storage[middle].id < id) {
				left = middle + 1;
			} else if (storage[middle].id > id) {
				right = middle - 1;
			} else {
				return &storage[middle];
			}
		}

		return nullptr;
	}

	Enemy* Stage::FindEnemy(unsigned int id)
	{
		return BinarySearch(enemies, id);
	}

	Bullet* Stage::FindBullet(unsigned int id)
	{
		return BinarySearch(bullets, id);
	}

	Player* Stage::FindPlayer(unsigned int id)
	{
		Player* result = nullptr;
		Player& player = player_storage;
		if (player.id == id) {
			result = &player;
		}
		return result;
	}

	Boss* Stage::FindBoss(unsigned int id)
	{
		Boss* result = nullptr;
		if (boss_exists) {
			Boss& boss = boss_storage;
			if (boss.id == id) {
				result = &boss;
			}
		}
		return result;
	}

	void Stage::FreeEnemy(Enemy& enemy)
	{
		if (enemy.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.coroutine);
		if (enemy.update_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.update_callback);
		if (enemy.death_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.death_callback);
	}

	void Stage::FreeBullet(Bullet& bullet)
	{
		if (bullet.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, bullet.coroutine);
		if (bullet.update_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, bullet.update_callback);
	}

	void Stage::FreePlayer(Player& player)
	{
	
	}

	void Stage::FreeBoss(Boss& boss)
	{
		if (boss.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, boss.coroutine);
		boss_exists = false;
	}

	float Stage::random_range(float a, float b)
	{
		float r = (float)rEngine() / (float)ULONG_MAX;
		float range = b - a;
		r = a + fmodf(range * r, range);
		return r;
	}
}
