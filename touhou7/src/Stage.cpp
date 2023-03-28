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

namespace th {

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
		float r = ctx->game_scene->stage.random_range(a, b);
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

	static void* lua_named_argp(lua_State* L, int argc, int i, const char* name, void* def = nullptr) {
		void* res = def;
		if (argc >= i) {
			lua_geti(L, 1, i);
			luaL_checktype(L, -1, LUA_TLIGHTUSERDATA);
			res = lua_touserdata(L, -1);
			lua_pop(L, 1);
		} else {
			lua_getfield(L, 1, name);
			if (!lua_isnil(L, -1)) {
				luaL_checktype(L, -1, LUA_TLIGHTUSERDATA);
				res = lua_touserdata(L, -1);
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

		float x   = lua_named_argf(L, argc, 1, "x", (float)PLAY_AREA_W / 2.0f);
		float y   = lua_named_argf(L, argc, 2, "y", 0.0f);
		float spd = lua_named_argf(L, argc, 3, "spd");
		float dir = lua_named_argf(L, argc, 4, "dir");
		float acc = lua_named_argf(L, argc, 5, "acc");
		void* spr = lua_named_argp(L, argc, 6, "spr");
		int drops = lua_named_argi(L, argc, 7, "drops");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 8, "Script")) {
			coroutine = CreateCoroutine(L, ctx->game_scene->stage.L); // it's possible to get the main thread without context
		}

		int death_callback = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 9, "OnDeath")) {
			death_callback = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		int update_callback = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 10, "OnUpdate")) {
			update_callback = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		Enemy& enemy = ctx->game_scene->stage.CreateEnemy();
		enemy.x = x;
		enemy.y = y;
		enemy.spd = spd;
		enemy.dir = cpml::angle_wrap(dir);
		enemy.acc = acc;
		enemy.radius = 10.0f;
		enemy.sc.sprite = (SpriteData*)spr;
		enemy.hp = 10.0f;
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
		int type  = lua_named_argi(L, argc, 6, "type");

		Game* ctx = lua_getcontext(L);

		BossData* data = GetBossData(type);

		Boss& boss = ctx->game_scene->stage.CreateBoss();
		boss.x = x;
		boss.y = y;
		boss.spd = spd;
		boss.dir = cpml::angle_wrap(dir);
		boss.acc = acc;
		boss.radius = 25.0f;
		boss.sc.sprite = data->sprite;
		boss.type_index = type;

		ctx->game_scene->stage.StartBossPhase();

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
		int type  = lua_named_argi(L, argc, 6, "type");
		int color = lua_named_argi(L, argc, 7, "color");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 8, "Script")) {
			coroutine = CreateCoroutine(L, ctx->game_scene->stage.L); // @main_thread_without_context
		}

		BulletData* data = GetBulletData(type);

		Bullet& bullet = ctx->game_scene->stage.CreateBullet();
		bullet.x = x;
		bullet.y = y;
		bullet.spd = spd;
		bullet.dir = cpml::angle_wrap(dir);
		bullet.acc = acc;

		bullet.type = ProjectileType::Bullet;
		bullet.radius = data->radius;
		bullet.rotate = data->rotate;

		bullet.sc.sprite = data->sprite;
		bullet.sc.frame_index = (float)color;
		bullet.coroutine = coroutine;
		lua_pushinteger(L, bullet.id);
		return 1;
	}

	static int lua_ShootLazer(lua_State* L) {
		int argc = lua_getargc(L);

		float x   = lua_named_argf(L, argc, 1, "x");
		float y   = lua_named_argf(L, argc, 2, "y");
		float spd = lua_named_argf(L, argc, 3, "spd");
		float dir = lua_named_argf(L, argc, 4, "dir");
		float length = lua_named_argf(L, argc, 5, "length");
		float thickness = lua_named_argf(L, argc, 6, "thickness");
		int color = lua_named_argi(L, argc, 7, "color");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, 8, "Script")) {
			coroutine = CreateCoroutine(L, ctx->game_scene->stage.L); // @main_thread_without_context
		}

		if (spd == 0.0f) spd = 1.0f;
		if (length == 0.0f) length = 1.0f;
		float time = length / spd;

		Bullet& bullet = ctx->game_scene->stage.CreateBullet();
		bullet.x = x;
		bullet.y = y;
		bullet.spd = spd;
		bullet.dir = cpml::angle_wrap(dir);

		bullet.type = ProjectileType::Lazer;
		bullet.target_length = length;
		bullet.thickness = thickness;
		bullet.lazer_time = time;

		bullet.sc.sprite = ctx->assets.GetSprite("Lazer");
		bullet.sc.frame_index = (float)color;
		bullet.coroutine = coroutine;
		lua_pushinteger(L, bullet.id);
		return 1;
	}

	static int lua_FindSprite(lua_State* L) {
		lua_checkargc(L, 1, 1);
		const char* name = luaL_checkstring(L, 1);
		Game* ctx = lua_getcontext(L);
		SpriteData* result = ctx->assets.GetSprite(name);
		lua_pushlightuserdata(L, result);
		return 1;
	}

	template <
		typename T,
		void (*PushFunc)(lua_State* L, T value),
		T (*GetFromBullet)(Bullet* bullet),
		T (*GetFromEnemy)(Enemy* enemy),
		T (*GetFromPlayer)(Player* player),
		T (*GetFromBoss)(Boss* boss),
		T (*GetFromStage)(void*) = nullptr
	>
	static int lua_GetObjectVar(lua_State* L) {
		lua_checkargc(L, 1, 1);
		instance_id id = (instance_id)luaL_checkinteger(L, 1);
		unsigned char type = id >> TYPE_PART_SHIFT;

		Game* ctx = lua_getcontext(L);
		T value{};
		switch (type) {
			case TYPE_BULLET: {
				if (Bullet* bullet = ctx->game_scene->stage.FindBullet(id)) {
					if (GetFromBullet != nullptr) value = GetFromBullet(bullet);
				}
				break;
			}
			case TYPE_ENEMY: {
				if (Enemy* enemy = ctx->game_scene->stage.FindEnemy(id)) {
					if (GetFromEnemy != nullptr) value = GetFromEnemy(enemy);
				}
				break;
			}
			case TYPE_PLAYER: {
				if (Player* player = ctx->game_scene->stage.FindPlayer(id)) {
					if (GetFromPlayer != nullptr) value = GetFromPlayer(player);
				}
				break;
			}
			case TYPE_BOSS: {
				if (Boss* boss = ctx->game_scene->stage.FindBoss(id)) {
					if (GetFromBoss != nullptr) value = GetFromBoss(boss);
				}
				break;
			}
			case 0xf: {
				if (GetFromStage != nullptr) value = GetFromStage(nullptr);
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
		instance_id id = (instance_id)luaL_checkinteger(L, 1);
		T value = ToFunc(L, 2);
		unsigned char type = id >> TYPE_PART_SHIFT;

		Game* ctx = lua_getcontext(L);
		switch (type) {
			case TYPE_BULLET: {
				if (Bullet* bullet = ctx->game_scene->stage.FindBullet(id)) {
					if (BulletSet != nullptr) BulletSet(bullet, value);
				}
				break;
			}
			case TYPE_ENEMY: {
				if (Enemy* enemy = ctx->game_scene->stage.FindEnemy(id)) {
					if (EnemySet != nullptr) EnemySet(enemy, value);
				}
				break;
			}
			case TYPE_PLAYER: {
				if (Player* player = ctx->game_scene->stage.FindPlayer(id)) {
					if (PlayerSet != nullptr) PlayerSet(player, value);
				}
				break;
			}
			case TYPE_BOSS: {
				if (Boss* boss = ctx->game_scene->stage.FindBoss(id)) {
					if (BossSet != nullptr) BossSet(boss, value);
				}
				break;
			}
		}
		return 0;
	}

	static int lua_Exists(lua_State* L) {
		lua_checkargc(L, 1, 1);
		instance_id id = (instance_id)luaL_checkinteger(L, 1);
		unsigned char type = id >> TYPE_PART_SHIFT;

		Game* ctx = lua_getcontext(L);
		bool result = false;
		switch (type) {
			case TYPE_BULLET: {
				result = ctx->game_scene->stage.FindBullet(id);
				break;
			}
			case TYPE_ENEMY: {
				result = ctx->game_scene->stage.FindEnemy(id);
				break;
			}
			case TYPE_PLAYER: {
				result = ctx->game_scene->stage.FindPlayer(id);
				break;
			}
			case TYPE_BOSS: {
				result = ctx->game_scene->stage.FindBoss(id);
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
	static void ObjectVarPushID(lua_State* L, instance_id value) { lua_pushinteger(L, value); }
	static void ObjectVarPushLUserdata(lua_State* L, void* value) { lua_pushlightuserdata(L, value); }

	static float ObjectVarToFloat(lua_State* L, int idx) { return (float)luaL_checknumber(L, idx); }
	static instance_id ObjectVarToID(lua_State* L, int idx) { return (instance_id)luaL_checkinteger(L, idx); }
	static void* ObjectVarToLUserdata(lua_State* L, int idx) { luaL_checktype(L, idx, LUA_TLIGHTUSERDATA); return lua_touserdata(L, idx); }

	template <typename Object>
	static float GetXFromObject(Object* object) { return object->x; }

	template <typename Object>
	static float GetYFromObject(Object* object) { return object->y; }

	template <typename Object>
	static float GetSpdFromObject(Object* object) { return object->spd; }
	static float GetSpdFromPlayer(Player* player) { return cpml::point_distance(0.0f, 0.0f, player->hsp, player->vsp); }

	template <typename Object>
	static float GetDirFromObject(Object* object) { return object->dir; }
	static float GetDirFromPlayer(Player* player) { return cpml::point_direction(0.0f, 0.0f, player->hsp, player->vsp); }

	template <typename Object>
	static float GetAccFromObject(Object* object) { return object->acc; }

	template <typename Object>
	static instance_id GetTargetFromObject(Object* object) { return 0 | (TYPE_PLAYER << TYPE_PART_SHIFT); }
	static instance_id GetTargetFromStage(void*) { return 0 | (TYPE_PLAYER << TYPE_PART_SHIFT); }

	template <typename Object>
	static void* GetSprFromObject(Object* object) { return object->sc.sprite; }
	template <typename Object>
	static float GetImgFromObject(Object* object) { return object->sc.frame_index; }

	template <typename Object>
	static void SetXForObject(Object* object, float value) { object->x = value; }

	template <typename Object>
	static void SetYForObject(Object* object, float value) { object->y = value; }

	template <typename Object>
	static void SetSpdForObject(Object* object, float value) { object->spd = value; }

	template <typename Object>
	static void SetDirForObject(Object* object, float value) { object->dir = cpml::angle_wrap(value); }

	template <typename Object>
	static void SetAccForObject(Object* object, float value) { object->acc = value; }

	template <typename Object>
	static void SetSprForObject(Object* object, void* value) { object->sc.sprite = (SpriteData*)value; }
	template <typename Object>
	static void SetImgForObject(Object* object, float value) { object->sc.frame_index = value; }

	bool Stage::Init() {
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
			lua_register(L, "ShootLazer", lua_ShootLazer);
			lua_register(L, "Exists", lua_Exists);
			lua_register(L, "FindSprite", lua_FindSprite);

			lua_CFunction GetX = lua_GetObjectVar<float, ObjectVarPushFloat, GetXFromObject<Bullet>, GetXFromObject<Enemy>, GetXFromObject<Player>, GetXFromObject<Boss>>;
			lua_CFunction GetY = lua_GetObjectVar<float, ObjectVarPushFloat, GetYFromObject<Bullet>, GetYFromObject<Enemy>, GetYFromObject<Player>, GetYFromObject<Boss>>;
			lua_CFunction GetSpd = lua_GetObjectVar<float, ObjectVarPushFloat, GetSpdFromObject<Bullet>, GetSpdFromObject<Enemy>, GetSpdFromPlayer, GetSpdFromObject<Boss>>;
			lua_CFunction GetDir = lua_GetObjectVar<float, ObjectVarPushFloat, GetDirFromObject<Bullet>, GetDirFromObject<Enemy>, GetDirFromPlayer, GetDirFromObject<Boss>>;
			lua_CFunction GetAcc = lua_GetObjectVar<float, ObjectVarPushFloat, GetAccFromObject<Bullet>, GetAccFromObject<Enemy>, nullptr, GetAccFromObject<Boss>>;
			lua_CFunction GetTarget = lua_GetObjectVar<instance_id, ObjectVarPushID, GetTargetFromObject<Bullet>, GetTargetFromObject<Enemy>, nullptr, GetTargetFromObject<Boss>, GetTargetFromStage>;
			lua_CFunction GetSpr = lua_GetObjectVar<void*, ObjectVarPushLUserdata, GetSprFromObject<Bullet>, GetSprFromObject<Enemy>, GetSprFromObject<Player>, GetSprFromObject<Boss>>;
			lua_CFunction GetImg = lua_GetObjectVar<float, ObjectVarPushFloat, GetImgFromObject<Bullet>, GetImgFromObject<Enemy>, GetImgFromObject<Player>, GetImgFromObject<Boss>>;

			lua_register(L, "GetX", GetX);
			lua_register(L, "GetY", GetY);
			lua_register(L, "GetSpd", GetSpd);
			lua_register(L, "GetDir", GetDir);
			lua_register(L, "GetAcc", GetAcc);
			lua_register(L, "GetTarget", GetTarget);
			lua_register(L, "GetSpr", GetSpr);
			lua_register(L, "GetImg", GetImg);

			lua_CFunction SetX = lua_SetObjectVar<float, ObjectVarToFloat, SetXForObject<Bullet>, SetXForObject<Enemy>, SetXForObject<Player>, SetXForObject<Boss>>;
			lua_CFunction SetY = lua_SetObjectVar<float, ObjectVarToFloat, SetYForObject<Bullet>, SetYForObject<Enemy>, SetYForObject<Player>, SetYForObject<Boss>>;
			lua_CFunction SetSpd = lua_SetObjectVar<float, ObjectVarToFloat, SetSpdForObject<Bullet>, SetSpdForObject<Enemy>, nullptr, SetSpdForObject<Boss>>;
			lua_CFunction SetDir = lua_SetObjectVar<float, ObjectVarToFloat, SetDirForObject<Bullet>, SetDirForObject<Enemy>, nullptr, SetDirForObject<Boss>>;
			lua_CFunction SetAcc = lua_SetObjectVar<float, ObjectVarToFloat, SetAccForObject<Bullet>, SetAccForObject<Enemy>, nullptr, SetAccForObject<Boss>>;
			lua_CFunction SetSpr = lua_SetObjectVar<void*, ObjectVarToLUserdata, SetSprForObject<Bullet>, SetSprForObject<Enemy>, SetSprForObject<Player>, SetSprForObject<Boss>>;
			lua_CFunction SetImg = lua_SetObjectVar<float, ObjectVarToFloat, SetImgForObject<Bullet>, SetImgForObject<Enemy>, SetImgForObject<Player>, SetImgForObject<Boss>>;

			lua_register(L, "SetX", SetX);
			lua_register(L, "SetY", SetY);
			lua_register(L, "SetSpd", SetSpd);
			lua_register(L, "SetDir", SetDir);
			lua_register(L, "SetAcc", SetAcc);
			lua_register(L, "SetSpr", SetSpr);
			lua_register(L, "SetImg", SetImg);

			lua_pushboolean(L, game.skip_to_midboss);
			lua_setglobal(L, "SKIP_TO_MIDBOSS");
			lua_pushboolean(L, game.skip_to_boss);
			lua_setglobal(L, "SKIP_TO_BOSS");
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
			StageData* data = GetStageData(scene.stage_index);

			lua_getglobal(L, data->script);
			coroutine = CreateCoroutine(L, L);
		}

		CreatePlayer();

		return true;
	}

	void Stage::Quit() {
		for (auto enemy = enemies.begin(); enemy != enemies.end(); ++enemy) {
			FreeEnemy(*enemy);
		}
		enemies.clear();

		for (auto bullet = bullets.begin(); bullet != bullets.end(); ++bullet) {
			FreeBullet(*bullet);
		}
		bullets.clear();

		FreePlayer();

		FreeBoss();

		lua_close(L); // crashes if L is null
	}

#define PLAYER_DEATH_TIME      20.0f
#define PLAYER_APPEAR_TIME     20.0f
#define PLAYER_RESPAWN_IFRAMES 120.0f
#define PLAYER_BOMB_TIME       (2.5f * 60.0f)

	void Stage::Update(float delta) {
		// update
		{
			UpdatePlayer(delta);

			if (boss_exists) {
				UpdateBoss(delta);
			}

			for (auto pickup = pickups.begin(); pickup != pickups.end(); ++pickup) {
				if (pickup->homing) {
					float spd = 8.0f;
					float dir = cpml::point_direction(pickup->x, pickup->y, player.x, player.y);
					pickup->hsp = cpml::lengthdir_x(spd, dir);
					pickup->vsp = cpml::lengthdir_y(spd, dir);
				} else {
					pickup->hsp = 0.0f;
					pickup->vsp += 0.025f;
					pickup->vsp = std::min(pickup->vsp, 2.0f);
				}
			}

			for (auto bullet = bullets.begin(); bullet != bullets.end(); ++bullet) {
				if (bullet->type == ProjectileType::Lazer) {
					if (bullet->lazer_timer < bullet->lazer_time) {
						bullet->lazer_timer += delta;
						bullet->length = cpml::lerp(0.0f, bullet->target_length, bullet->lazer_timer / bullet->lazer_time);
					} else {
						bullet->length = bullet->target_length;
					}
				}
			}

			for (PlayerBullet& bullet : player_bullets) {
				switch (bullet.type) {
					case PLAYER_BULLET_REIMU_CARD: {
						bullet.rotation += 16.0f * delta;
						break;
					}
					case PLAYER_BULLET_REIMU_ORB_SHOT: {
						// home
						break;
					}
				}
			}
		}

		// physics
		{
			float physics_update_rate = 1.0f / 300.0f; // 300 fps
			float physics_timer = delta * 1.0f;//g_stage->gameplay_delta;
			while (physics_timer > 0.0f) {
				float pdelta = std::min(physics_timer, physics_update_rate * 60.0f);
				PhysicsUpdate(pdelta);
				physics_timer -= pdelta;
			}
		}

		// coroutines
		{
			coro_update_timer += delta;
			while (coro_update_timer >= 1.0f) {
				CallCoroutines();
				coro_update_timer -= 1.0f;
			}
		}

		// late update
		{
			player.x = std::clamp(player.x, 0.0f, (float)PLAY_AREA_W - 1.0f);
			player.y = std::clamp(player.y, 0.0f, (float)PLAY_AREA_H - 1.0f);

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

			for (auto pickup = pickups.begin(); pickup != pickups.end();) {
				float x = pickup->x;
				float y = pickup->y;
				if (x < 0.0f || x >= (float)PLAY_AREA_W || y < -50.0f || y >= (float)PLAY_AREA_H) {
					pickup = pickups.erase(pickup);
					continue;
				}
				++pickup;
			}

			for (auto b = player_bullets.begin(); b != player_bullets.end();) {
				float x = b->x;
				float y = b->y;
				if (x < 0.0f || x >= (float)PLAY_AREA_W || y < 0.0f || y >= (float)PLAY_AREA_H) {
					b = player_bullets.erase(b);
					continue;
				}
				++b;
			}
		}

		{
			if (boss_exists) {
				UpdateSpriteComponent(boss.sc, delta);
			}

			for (auto enemy = enemies.begin(); enemy != enemies.end(); ++enemy) {
				UpdateSpriteComponent(enemy->sc, delta);
			}
		}

		time += delta;
	}

	void Stage::UpdatePlayer(float delta) {
		const unsigned char* key = SDL_GetKeyboardState(nullptr);

		if (game.key_pressed[SDL_SCANCODE_P]) scene.stats.power += 8;

		switch (player.state) {
			case PlayerState::Normal: {
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

				CharacterData* character = GetCharacterData(game.player_character);

				float spd = player.focus ? character->focus_spd : character->move_spd;
				player.hsp = xmove * spd;
				player.vsp = ymove * spd;

				if (character->shot_type) {
					(*character->shot_type)(&game, delta);
				}

				if (key[SDL_SCANCODE_X]) {
					if (player.bomb_timer == 0.f) {
						if (scene.stats.bombs > 0) {
							if (character->bomb) {
								(*character->bomb)(&game);
							}
							scene.stats.bombs--;
							player.bomb_timer = PLAYER_BOMB_TIME;
						}
					}
				}

				player.iframes = std::max(player.iframes - delta, 0.0f);

				SpriteComponent& sc = player.sc;

				if (xmove != 0.0f) {
					if (sc.sprite != character->turn_spr) {
						sc.sprite = character->turn_spr;
						sc.frame_index = 0.0f;
					}
					if (xmove > 0.0f) {
						player.facing = 1.0f;
					} else {
						player.facing = -1.0f;
					}
				}

				if (xmove == 0.0f && sc.sprite == character->turn_spr) {
					if (sc.frame_index > (float)sc.sprite->loop_frame - 1.0f) {
						sc.frame_index = (float)sc.sprite->loop_frame - 1.0f;
					}
					sc.frame_index -= sc.sprite->anim_spd * delta;
					if (sc.frame_index < 0.0f) {
						sc.sprite = character->idle_spr;
						sc.frame_index = 0.0f;
					}
				} else {
					sc.frame_index += sc.sprite->anim_spd * delta;
					if ((int)sc.frame_index >= sc.sprite->frame_count) {
						SpriteData* sprite = sc.sprite;
						sc.frame_index = (float)sprite->loop_frame + fmodf(sc.frame_index - (float)sprite->loop_frame, (float)(sprite->frame_count - sprite->loop_frame));
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

	void Stage::UpdateSpriteComponent(SpriteComponent& sc, float delta) {
		if (SpriteData* sprite = sc.sprite) {
			int frame_count = sprite->frame_count;
			int loop_frame = sprite->loop_frame;
			float anim_spd = sprite->anim_spd;

			sc.frame_index += anim_spd * delta;
			if ((int)sc.frame_index >= frame_count) {
				sc.frame_index = (float)loop_frame + fmodf(sc.frame_index - (float)loop_frame, (float)(frame_count - loop_frame));
			}
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

	void Stage::PhysicsUpdate(float delta) {
		{
			player.x += player.hsp * delta;
			player.y += player.vsp * delta;

			if (boss_exists) {
				MoveObject(boss, delta);
			}

			for (auto enemy = enemies.begin(); enemy != enemies.end(); ++enemy) {
				MoveObject(*enemy, delta);
			}

			for (auto bullet = bullets.begin(); bullet != bullets.end(); ++bullet) {
				if (bullet->type == ProjectileType::Lazer) {
					if (bullet->lazer_timer >= bullet->lazer_time) {
						MoveObject(*bullet, delta);
					}
				} else {
					MoveObject(*bullet, delta);
				}
			}

			for (auto pickup = pickups.begin(); pickup != pickups.end(); ++pickup) {
				pickup->x += pickup->hsp * delta;
				pickup->y += pickup->vsp * delta;
			}

			for (PlayerBullet& b : player_bullets) {
				MoveObject(b, delta);
			}
		}

		{
			CharacterData* character = GetCharacterData(game.player_character);

			// player vs bullet

			// player vs pickup
			for (auto pickup = pickups.begin(); pickup != pickups.end();) {
				if (cpml::circle_vs_circle(player.x, player.y, character->graze_radius, pickup->x, pickup->y, pickup->radius)) {
					if (player.state == PlayerState::Normal) {
						switch (pickup->type) {
							case PICKUP_POWER:      /*g_screen->get_power (i, 1);        */ break;
							case PICKUP_POINT:      /*g_screen->get_points(i, 1);        */ break;
							case PICKUP_BIGP:       /*g_screen->get_power (i, 8);        */ break;
							case PICKUP_BOMB:       /*g_screen->get_bombs (i, 1);        */ break;
							case PICKUP_FULL_POWER: /*g_screen->get_power (i, MAX_POWER);*/ break;
							case PICKUP_1UP:        /*g_screen->get_lives (i, 1);        */ break;
							case PICKUP_SCORE:      /*g_screen->get_score (i, 10);       */ break;
						}

						pickup = pickups.erase(pickup);
						continue;
					}
				}
				++pickup;
			}

			// boss vs bullet
			if (boss_exists) {
				for (auto bullet = player_bullets.begin(); bullet != player_bullets.end();) {
					if (cpml::circle_vs_circle(boss.x, boss.y, boss.radius, bullet->x, bullet->y, bullet->radius)) {
						if (boss.state == BossState::Normal) {
							boss.hp -= bullet->dmg;
							bullet = player_bullets.erase(bullet);
							if (boss.hp <= 0.0f) {
								EndBossPhase();
								break;
							}
							continue;
						} else {
							bullet = player_bullets.erase(bullet);
							continue;
						}
					}
					++bullet;
				}
			}

			// enemy vs bullet
			for (auto enemy = enemies.begin(); enemy != enemies.end();) {
				bool enemy_dead = false;
				for (auto bullet = player_bullets.begin(); bullet != player_bullets.end();) {
					if (cpml::circle_vs_circle(enemy->x, enemy->y, enemy->radius, bullet->x, bullet->y, bullet->radius)) {
						enemy->hp -= bullet->dmg;
						bullet = player_bullets.erase(bullet);
						if (enemy->hp <= 0.0f) {
							enemy_dead = true;
							break;
						}
						continue;
					}
					++bullet;
				}
				if (enemy_dead) {
					enemy = enemies.erase(enemy);
					continue;
				}
				++enemy;
			}
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

	void Stage::StartBossPhase() {
		BossData* data = GetBossData(boss.type_index);
		PhaseData* phase = GetPhaseData(data, boss.phase_index);

		boss.hp = phase->hp;
		boss.timer = phase->time;
		boss.wait_timer = BOSS_PHASE_START_TIME;
		boss.state = BossState::WaitingStart;

		if (boss.phase_index > 0) {
			LaunchTowardsPoint(boss, BOSS_STARTING_X, BOSS_STARTING_Y, 0.05);
		}
	}

	bool Stage::EndBossPhase() {
		for (auto bullet = bullets.begin(); bullet != bullets.end(); ++bullet) {
			Pickup& pickup = CreatePickup(bullet->x, bullet->y, PICKUP_SCORE);
			pickup.homing = true;

			FreeBullet(*bullet);
		}
		bullets.clear();

		if (boss.coroutine != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, boss.coroutine);
			boss.coroutine = LUA_REFNIL;
		}

		BossData* data = GetBossData(boss.type_index);
		PhaseData* phase = GetPhaseData(data, boss.phase_index);

		if (phase->type == PHASE_SPELLCARD) {
			// drop some pickups
			for (int i = 0; i < 5; i++) {
				float x = boss.x + random_range(-50.f, 50.f);
				float y = boss.y + random_range(-50.f, 50.f);
				unsigned char type = (i == 4) ? PICKUP_BIGP : PICKUP_POWER;
				CreatePickup(x, y, type);
			}
		}

		if (boss.phase_index + 1 < data->phase_count) {
			if (phase->type == PHASE_SPELLCARD) {
				boss.state = BossState::WaitingEnd;
				boss.wait_timer = BOSS_SPELL_END_TIME;
			} else {
				boss.phase_index++;
				StartBossPhase();
			}
		} else {
			for (auto pickup = pickups.begin(); pickup != pickups.end(); ++pickup) {
				pickup->homing = true;
			}

			FreeBoss();
			boss_exists = false;
		}

		return true;
	}

	void Stage::UpdateBoss(float delta) {
		switch (boss.state) {
			case BossState::Normal: {
				boss.timer = std::max(boss.timer - delta, 0.0f);
				if (boss.timer == 0.f) {
					EndBossPhase();

					if (!boss_exists) {
						return;
					}
				}
				break;
			}
			case BossState::WaitingStart: {
				boss.wait_timer = std::max(boss.wait_timer - delta, 0.0f);
				if (boss.wait_timer == 0.f) {
					BossData* data = GetBossData(boss.type_index);
					PhaseData* phase = GetPhaseData(data, boss.phase_index);

					boss.state = BossState::Normal;
					lua_getglobal(L, phase->script);
					boss.coroutine = CreateCoroutine(L, L);
				}
				break;
			}
			case BossState::WaitingEnd: {
				boss.wait_timer = std::max(boss.wait_timer - delta, 0.0f);
				if (boss.wait_timer == 0.0f) {
					boss.phase_index++;
					StartBossPhase();
				}
				break;
			}
		}

		if (boss.spd > 0.1f) {
			if (90.0f <= boss.dir && boss.dir < 270.0f) {
				boss.facing = 1.0f;
			} else {
				boss.facing = -1.0f;
			}
		}

#ifdef TH_DEBUG
		if (game.key_pressed[SDL_SCANCODE_B]) {
			EndBossPhase();

			if (boss_exists) {
				//if (!IsKeyDown(KEY_LEFT_CONTROL)) {
					if (boss.state == BossState::WaitingEnd) {
						boss.wait_timer = 0.0f;
					}
				//}
			}
		}
#endif

		//std::cout << "hp " << boss.hp << std::endl;
		//std::cout << "timer " << boss.timer << std::endl;
		//std::cout << "wait timer " << boss.wait_timer << std::endl;
		//std::cout << "phase " << boss.phase_index << std::endl;
	}

	static void UpdateCoroutine(lua_State* L, int* coroutine, instance_id id) {
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

	void Stage::CallCoroutines() {
		UpdateCoroutine(L, &coroutine, -1);

		if (boss_exists) {
			UpdateCoroutine(L, &boss.coroutine, boss.id);
		}

		for (auto enemy = enemies.begin(); enemy != enemies.end(); ++enemy) {
			UpdateCoroutine(L, &enemy->coroutine, enemy->id);
		}

		for (auto bullet = bullets.begin(); bullet != bullets.end(); ++bullet) {
			UpdateCoroutine(L, &bullet->coroutine, bullet->id);
		}
	}

	template <typename Object>
	static void DrawObject(SDL_Renderer* renderer, const Object& object, float angle = 0.0f, float xscale = 1.0f, float yscale = 1.0f, SDL_Color color = {255, 255, 255, 255}) {
		float x = object.x;
		float y = object.y;
		SpriteData* sprite = object.sc.sprite;
		int frame_index = (int)object.sc.frame_index;
		DrawSprite(renderer, sprite, frame_index, x, y, angle, xscale, yscale, color);
	}

	void Stage::Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta) {
		SDL_SetRenderTarget(renderer, target);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			// bg
			{
				SDL_Texture* texture = game.assets.GetTexture("MistyLakeTexture.png");
				SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
				{
					SDL_Rect dest{0, (int)time / 4 % PLAY_AREA_W - PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
					SDL_RenderCopy(renderer, texture, nullptr, &dest);
				}
				{
					SDL_Rect dest{0, (int)time / 4 % PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
					SDL_RenderCopy(renderer, texture, nullptr, &dest);
				}
				{
					SDL_Rect dest{0, (int)time / 4 % PLAY_AREA_W + PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
					SDL_RenderCopy(renderer, texture, nullptr, &dest);
				}
			}

			for (auto enemy = enemies.begin(); enemy != enemies.end(); ++enemy) {
				DrawObject(renderer, *enemy);
			}

			if (boss_exists) {
				DrawObject(renderer, boss, 0.0f, -boss.facing, 1.0f);
			}

			// player
			{
				SDL_Color color = {255, 255, 255, 255};
				if (player.iframes > 0.0f) {
					if (((int)time / 4) % 2) {
						color.a /= 2;
					}
				}
				DrawObject(renderer, player, 0.0f, -player.facing, 1.0f, color);
				if (player.hitbox_alpha > 0.0f) {
					SpriteData* sprite = game.assets.GetSprite("Hitbox");
					unsigned char a = (unsigned char) (255.0f * player.hitbox_alpha);
					DrawSprite(renderer, sprite, 0, player.x, player.y, -time, 1.0f, 1.0f, {255, 255, 255, a});
				}
			}

			for (auto pickup = pickups.begin(); pickup != pickups.end(); ++pickup) {
				DrawObject(renderer, *pickup);
			}

			for (PlayerBullet& b : player_bullets) {
				SDL_Color color = {255, 255, 255, 80};
				switch (b.type) {
					case PLAYER_BULLET_REIMU_CARD: {
						DrawObject(renderer, b, b.rotation, 1.5f, 1.5f, color);
						break;
					}
					case PLAYER_BULLET_REIMU_ORB_SHOT: {
						DrawObject(renderer, b, b.dir, 1.5f, 1.5f, color);
						break;
					}
				}
			}

			for (auto bullet = bullets.begin(); bullet != bullets.end(); ++bullet) {
				if (bullet->type == ProjectileType::Lazer) {
					float angle = bullet->dir + 90.0f;
					float xscale = (bullet->thickness + 2.0f) / 16.0f;
					float yscale = bullet->length / 16.0f;
					DrawObject(renderer, *bullet, angle, xscale, yscale);
				} else {
					float angle = 0.0f;
					if (bullet->rotate) {
						angle = bullet->dir - 90.0f;
					}
					DrawObject(renderer, *bullet, angle);
				}
			}


			// GUI
			{
				// pickup labels
				for (Pickup& pickup : pickups) {
					if (pickup.y < 0.0f) {
						SpriteData* sprite = pickup.sc.sprite;
						int frame_index = pickup.type + PICKUP_COUNT;
						float x = pickup.x;
						float y = 8.0f;
						SDL_Color color{255, 255, 255, 128};
						DrawSprite(renderer, sprite, frame_index, x, y, 0.0f, 1.0f, 1.0f, color);
					}
				}

				if (boss_exists) {
					BossData* data = GetBossData(boss.type_index);
					PhaseData* phase = GetPhaseData(data, boss.phase_index);

					// phases left
					{
						char buf[3];
						snprintf(buf, 3, "%d", data->phase_count - boss.phase_index - 1);
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
						int reduced_w = (int) ((float)healthbar_w * (boss.hp / phase->hp));
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

	Enemy& Stage::CreateEnemy() {
		Enemy& enemy = enemies.emplace_back();
		enemy.id = (next_id++) | (TYPE_ENEMY << TYPE_PART_SHIFT);
		return enemy;
	}

	Bullet& Stage::CreateBullet() {
		Bullet& bullet = bullets.emplace_back();
		bullet.id = (next_id++) | (TYPE_BULLET << TYPE_PART_SHIFT);
		return bullet;
	}

	Player& Stage::CreatePlayer(bool from_death) {
		player = {};
		player.id = 0 | (TYPE_PLAYER << TYPE_PART_SHIFT);
		player.x = PLAYER_STARTING_X;
		player.y = PLAYER_STARTING_Y;
		player.sc.sprite = game.assets.GetSprite("ReimuIdle");

		player.iframes = PLAYER_RESPAWN_IFRAMES;
		if (from_death) {
			player.state = PlayerState::Appearing;
			player.timer = PLAYER_APPEAR_TIME;
		}

		return player;
	}

	Boss& Stage::CreateBoss() {
		if (boss_exists) {
			FreeBoss();
		}
		boss = {};
		boss.id = 0 | (TYPE_BOSS << TYPE_PART_SHIFT);
		boss_exists = true;
		return boss;
	}

	Pickup& Stage::CreatePickup(float x, float y, unsigned char type) {
		Pickup& pickup = pickups.emplace_back();
		pickup.x = x;
		pickup.y = y;
		pickup.vsp = -1.5f;
		pickup.radius = 8.0f;
		pickup.sc.sprite = game.assets.GetSprite("Pickup");
		pickup.sc.frame_index = (float)type;
		pickup.type = type;
		return pickup;
	}

	PlayerBullet& Stage::CreatePlayerBullet() {
		PlayerBullet& player_bullet = player_bullets.emplace_back();
		return player_bullet;
	}

	typedef ptrdiff_t ssize;

	template <typename T>
	static T* BinarySearch(std::vector<T>& storage, instance_id id) {
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

	Enemy* Stage::FindEnemy(instance_id id) {
		return BinarySearch(enemies, id);
	}

	Bullet* Stage::FindBullet(instance_id id) {
		return BinarySearch(bullets, id);
	}

	Player* Stage::FindPlayer(instance_id id) {
		Player* result = nullptr;
		if (player.id == id) {
			result = &player;
		}
		return result;
	}

	Boss* Stage::FindBoss(instance_id id) {
		Boss* result = nullptr;
		if (boss_exists) {
			if (boss.id == id) {
				result = &boss;
			}
		}
		return result;
	}

	void Stage::FreeEnemy(Enemy& enemy) {
		if (enemy.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.coroutine);
		if (enemy.update_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.update_callback);
		if (enemy.death_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.death_callback);
	}

	void Stage::FreeBullet(Bullet& bullet) {
		if (bullet.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, bullet.coroutine);
		if (bullet.update_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, bullet.update_callback);
	}

	void Stage::FreePlayer() {
	
	}

	void Stage::FreeBoss() {
		if (boss.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, boss.coroutine);
	}

	float Stage::random_range(float a, float b) {
		float r = (float)rEngine() / (float)ULONG_MAX;
		float range = b - a;
		r = a + fmodf(range * r, range);
		return r;
	}

}
