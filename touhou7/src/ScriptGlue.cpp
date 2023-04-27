#include "Game.h"

#include "common.h"
#include "cpml.h"
#include "external/stb_sprintf.h"

namespace th {

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
		float r = ctx->game_scene->stage->random.range(a, b);
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

	// @main_thread
	int CreateCoroutine(lua_State* L, lua_State* main_L) {
		if (lua_gettop(L) < 1) {
			TH_LOG_ERROR("CreateCoroutine: stack is empty");
			return LUA_REFNIL;
		}

		if (!lua_isfunction(L, -1)) {
			TH_LOG_ERROR("CreateCoroutine: not a function");
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

		int i = 1;
		float x   = lua_named_argf(L, argc, i++, "x", (float)PLAY_AREA_W / 2.0f);
		float y   = lua_named_argf(L, argc, i++, "y", 0.0f);
		float spd = lua_named_argf(L, argc, i++, "spd");
		float dir = lua_named_argf(L, argc, i++, "dir");
		float acc = lua_named_argf(L, argc, i++, "acc");
		void* spr = lua_named_argp(L, argc, i++, "spr");
		int drops = lua_named_argi(L, argc, i++, "drops");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, i++, "Script")) {
			coroutine = CreateCoroutine(L, ctx->game_scene->stage->L); // @main_thread
		}

		int death_callback = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, i++, "OnDeath")) {
			death_callback = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		int update_callback = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, i++, "OnUpdate")) {
			update_callback = luaL_ref(L, LUA_REGISTRYINDEX);
		}

		Enemy& enemy = ctx->game_scene->stage->CreateEnemy();
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

		int i = 1;
		float x   = lua_named_argf(L, argc, i++, "x", BOSS_STARTING_X);
		float y   = lua_named_argf(L, argc, i++, "y", BOSS_STARTING_Y);
		float spd = lua_named_argf(L, argc, i++, "spd");
		float dir = lua_named_argf(L, argc, i++, "dir");
		float acc = lua_named_argf(L, argc, i++, "acc");
		int type  = lua_named_argi(L, argc, i++, "type");

		Game* ctx = lua_getcontext(L);

		BossData* data = GetBossData(type);

		Boss& boss = ctx->game_scene->stage->CreateBoss();
		boss.x = x;
		boss.y = y;
		boss.spd = spd;
		boss.dir = cpml::angle_wrap(dir);
		boss.acc = acc;
		boss.radius = 25.0f;
		boss.sc.sprite = data->sprite;
		boss.type_index = type;

		ctx->game_scene->stage->StartBossPhase();

		lua_pushinteger(L, boss.id);
		return 1;
	}

	static int lua_Shoot(lua_State* L) {
		int argc = lua_getargc(L);

		int i = 1;
		float x   = lua_named_argf(L, argc, i++, "x");
		float y   = lua_named_argf(L, argc, i++, "y");
		float spd = lua_named_argf(L, argc, i++, "spd");
		float dir = lua_named_argf(L, argc, i++, "dir");
		float acc = lua_named_argf(L, argc, i++, "acc");
		int type  = lua_named_argi(L, argc, i++, "type");
		int color = lua_named_argi(L, argc, i++, "color");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, i++, "Script")) {
			coroutine = CreateCoroutine(L, ctx->game_scene->stage->L); // @main_thread
		}

		BulletData* data = GetBulletData(type);

		Bullet& bullet = ctx->game_scene->stage->CreateBullet();
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

		Mix_Chunk* sound = ctx->assets.GetSound("se_enemy_shoot.wav");
		StopSound(sound);
		Mix_PlayChannel(-1, sound, 0);

		lua_pushinteger(L, bullet.id);
		return 1;
	}

	static int lua_ShootLazer(lua_State* L) {
		int argc = lua_getargc(L);

		int i = 1;
		float x   = lua_named_argf(L, argc, i++, "x");
		float y   = lua_named_argf(L, argc, i++, "y");
		float spd = lua_named_argf(L, argc, i++, "spd");
		float dir = lua_named_argf(L, argc, i++, "dir");
		float length = lua_named_argf(L, argc, i++, "length");
		float thickness = lua_named_argf(L, argc, i++, "thickness");
		int color = lua_named_argi(L, argc, i++, "color");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, i++, "Script")) {
			coroutine = CreateCoroutine(L, ctx->game_scene->stage->L); // @main_thread
		}

		if (spd == 0.0f) spd = 1.0f;
		if (length == 0.0f) length = 1.0f;
		float time = length / spd;

		Bullet& bullet = ctx->game_scene->stage->CreateBullet();
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

		Mix_Chunk* sound = ctx->assets.GetSound("se_lazer.wav");
		StopSound(sound);
		Mix_PlayChannel(-1, sound, 0);

		lua_pushinteger(L, bullet.id);
		return 1;
	}

	static int lua_ShootSLazer(lua_State* L) {
		int argc = lua_getargc(L);

		int i = 1;
		float x   = lua_named_argf(L, argc, i++, "x");
		float y   = lua_named_argf(L, argc, i++, "y");
		float dir = lua_named_argf(L, argc, i++, "dir");
		float prep_time = lua_named_argf(L, argc, i++, "wait_time");
		float time = lua_named_argf(L, argc, i++, "lifespan");
		float thickness = lua_named_argf(L, argc, i++, "thickness");
		int color = lua_named_argi(L, argc, i++, "color");

		Game* ctx = lua_getcontext(L);

		int coroutine = LUA_REFNIL;
		if (lua_named_argfunc(L, argc, i++, "Script")) {
			coroutine = CreateCoroutine(L, ctx->game_scene->stage->L); // @main_thread
		}

		Bullet& bullet = ctx->game_scene->stage->CreateBullet();
		bullet.x = x;
		bullet.y = y;
		bullet.dir = cpml::angle_wrap(dir);

		bullet.type = ProjectileType::SLazer;
		bullet.target_length = 1'000.0f;
		bullet.thickness = thickness;
		bullet.lazer_time = prep_time;
		bullet.lazer_lifetime = prep_time + time;

		bullet.sc.sprite = ctx->assets.GetSprite("Lazer");
		bullet.sc.frame_index = (float)color;
		bullet.coroutine = coroutine;

		lua_pushinteger(L, bullet.id);
		return 1;
	}

	static int lua_FindSprite(lua_State* L) {
		lua_checkargc(L, 1, 1);
		//size_t size;
		//const char* name = luaL_checklstring(L, 1, &size);
		//std::string_view s(name, size);
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
	> static int lua_GetObjectVar(lua_State* L) {
		lua_checkargc(L, 1, 1);
		instance_id id = (instance_id)luaL_checkinteger(L, 1);
		unsigned char type = id >> TYPE_PART_SHIFT;

		Game* ctx = lua_getcontext(L);
		T value{};
		switch (type) {
			case TYPE_BULLET: {
				if (Bullet* bullet = ctx->game_scene->stage->FindBullet(id)) {
					if (GetFromBullet != nullptr) value = GetFromBullet(bullet);
				}
				break;
			}
			case TYPE_ENEMY: {
				if (Enemy* enemy = ctx->game_scene->stage->FindEnemy(id)) {
					if (GetFromEnemy != nullptr) value = GetFromEnemy(enemy);
				}
				break;
			}
			case TYPE_PLAYER: {
				if (Player* player = ctx->game_scene->stage->FindPlayer(id)) {
					if (GetFromPlayer != nullptr) value = GetFromPlayer(player);
				}
				break;
			}
			case TYPE_BOSS: {
				if (Boss* boss = ctx->game_scene->stage->FindBoss(id)) {
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
	> static int lua_SetObjectVar(lua_State* L) {
		lua_checkargc(L, 2, 2);
		instance_id id = (instance_id)luaL_checkinteger(L, 1);
		T value = ToFunc(L, 2);
		unsigned char type = id >> TYPE_PART_SHIFT;

		Game* ctx = lua_getcontext(L);
		switch (type) {
			case TYPE_BULLET: {
				if (Bullet* bullet = ctx->game_scene->stage->FindBullet(id)) {
					if (BulletSet != nullptr) BulletSet(bullet, value);
				}
				break;
			}
			case TYPE_ENEMY: {
				if (Enemy* enemy = ctx->game_scene->stage->FindEnemy(id)) {
					if (EnemySet != nullptr) EnemySet(enemy, value);
				}
				break;
			}
			case TYPE_PLAYER: {
				if (Player* player = ctx->game_scene->stage->FindPlayer(id)) {
					if (PlayerSet != nullptr) PlayerSet(player, value);
				}
				break;
			}
			case TYPE_BOSS: {
				if (Boss* boss = ctx->game_scene->stage->FindBoss(id)) {
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
				result = ctx->game_scene->stage->FindBullet(id);
				break;
			}
			case TYPE_ENEMY: {
				result = ctx->game_scene->stage->FindEnemy(id);
				break;
			}
			case TYPE_PLAYER: {
				result = ctx->game_scene->stage->FindPlayer(id);
				break;
			}
			case TYPE_BOSS: {
				result = ctx->game_scene->stage->FindBoss(id);
				break;
			}
		}
		lua_pushboolean(L, result);
		return 1;
	}

	static int lua_Destroy(lua_State* L) {
		lua_checkargc(L, 1, 1);
		instance_id id = (instance_id)luaL_checkinteger(L, 1);
		unsigned char type = id >> TYPE_PART_SHIFT;

		Game* ctx = lua_getcontext(L);
		switch (type) {
			case TYPE_BULLET: {
				if (Bullet* bullet = ctx->game_scene->stage->FindBullet(id)) {
					bullet->dead = true;
				}
				break;
			}
			case TYPE_ENEMY: {
				if (Enemy* enemy = ctx->game_scene->stage->FindEnemy(id)) {
					enemy->dead = true;
				}
				break;
			}
			case TYPE_PLAYER: {
				if (Player* player = ctx->game_scene->stage->FindPlayer(id)) {
					player->dead = true;
				}
				break;
			}
			case TYPE_BOSS: {
				if (Boss* boss = ctx->game_scene->stage->FindBoss(id)) {
					boss->dead = true;
				}
				break;
			}
		}
		return 0;
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
	static float GetAngleFromObject(Object* object) { return object->angle; }


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
	template <typename Object>
	static void SetAngleForObject(Object* object, float value) { object->angle = value; }

#if 0
	static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
		if (nsize == 0) {
			//printf("lua free %d bytes\n", (int)osize);
			printf("FREE ");
			free(ptr);
			return NULL;
		} else if (ptr == nullptr) {
			const char* s = "something else";
			switch (osize) {
				case LUA_TSTRING:   s = "string";   break;
				case LUA_TTABLE:    s = "table";    break;
				case LUA_TFUNCTION: s = "function"; break;
				case LUA_TUSERDATA: s = "userdata"; break;
				case LUA_TTHREAD:   s = "thread";   break;
			}
			//printf("lua alloc %d bytes (%s)\n", (int)nsize, s);
			printf("ALLOC ");
			return malloc(nsize);
		} else {
			//printf("lua realloc from %d bytes to %d bytes\n", (int)osize, (int)nsize);
			printf("REALLOC ");
			return realloc(ptr, nsize);
		}
	}
#endif

	void Stage::InitLua() {
		if (!(L = luaL_newstate())) {
			TH_SHOW_ERROR("luaL_newstate failed");
		}

		//lua_setallocf(L, l_alloc, nullptr);

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
			lua_register(L, "ShootSLazer", lua_ShootSLazer);
			lua_register(L, "Exists", lua_Exists);
			lua_register(L, "FindSprite", lua_FindSprite);
			lua_register(L, "Destroy", lua_Destroy);

			lua_CFunction GetX = lua_GetObjectVar<float, ObjectVarPushFloat, GetXFromObject<Bullet>, GetXFromObject<Enemy>, GetXFromObject<Player>, GetXFromObject<Boss>>;
			lua_CFunction GetY = lua_GetObjectVar<float, ObjectVarPushFloat, GetYFromObject<Bullet>, GetYFromObject<Enemy>, GetYFromObject<Player>, GetYFromObject<Boss>>;
			lua_CFunction GetSpd = lua_GetObjectVar<float, ObjectVarPushFloat, GetSpdFromObject<Bullet>, GetSpdFromObject<Enemy>, GetSpdFromPlayer, GetSpdFromObject<Boss>>;
			lua_CFunction GetDir = lua_GetObjectVar<float, ObjectVarPushFloat, GetDirFromObject<Bullet>, GetDirFromObject<Enemy>, GetDirFromPlayer, GetDirFromObject<Boss>>;
			lua_CFunction GetAcc = lua_GetObjectVar<float, ObjectVarPushFloat, GetAccFromObject<Bullet>, GetAccFromObject<Enemy>, nullptr, GetAccFromObject<Boss>>;
			lua_CFunction GetTarget = lua_GetObjectVar<instance_id, ObjectVarPushID, GetTargetFromObject<Bullet>, GetTargetFromObject<Enemy>, nullptr, GetTargetFromObject<Boss>, GetTargetFromStage>;
			lua_CFunction GetSpr = lua_GetObjectVar<void*, ObjectVarPushLUserdata, GetSprFromObject<Bullet>, GetSprFromObject<Enemy>, GetSprFromObject<Player>, GetSprFromObject<Boss>>;
			lua_CFunction GetImg = lua_GetObjectVar<float, ObjectVarPushFloat, GetImgFromObject<Bullet>, GetImgFromObject<Enemy>, GetImgFromObject<Player>, GetImgFromObject<Boss>>;
			lua_CFunction GetAngle = lua_GetObjectVar<float, ObjectVarPushFloat, nullptr, GetAngleFromObject<Enemy>, nullptr, nullptr>;

			lua_register(L, "GetX", GetX);
			lua_register(L, "GetY", GetY);
			lua_register(L, "GetSpd", GetSpd);
			lua_register(L, "GetDir", GetDir);
			lua_register(L, "GetAcc", GetAcc);
			lua_register(L, "GetTarget", GetTarget);
			lua_register(L, "GetSpr", GetSpr);
			lua_register(L, "GetImg", GetImg);
			lua_register(L, "GetAngle", GetAngle);

			lua_CFunction SetX = lua_SetObjectVar<float, ObjectVarToFloat, SetXForObject<Bullet>, SetXForObject<Enemy>, SetXForObject<Player>, SetXForObject<Boss>>;
			lua_CFunction SetY = lua_SetObjectVar<float, ObjectVarToFloat, SetYForObject<Bullet>, SetYForObject<Enemy>, SetYForObject<Player>, SetYForObject<Boss>>;
			lua_CFunction SetSpd = lua_SetObjectVar<float, ObjectVarToFloat, SetSpdForObject<Bullet>, SetSpdForObject<Enemy>, nullptr, SetSpdForObject<Boss>>;
			lua_CFunction SetDir = lua_SetObjectVar<float, ObjectVarToFloat, SetDirForObject<Bullet>, SetDirForObject<Enemy>, nullptr, SetDirForObject<Boss>>;
			lua_CFunction SetAcc = lua_SetObjectVar<float, ObjectVarToFloat, SetAccForObject<Bullet>, SetAccForObject<Enemy>, nullptr, SetAccForObject<Boss>>;
			lua_CFunction SetSpr = lua_SetObjectVar<void*, ObjectVarToLUserdata, SetSprForObject<Bullet>, SetSprForObject<Enemy>, SetSprForObject<Player>, SetSprForObject<Boss>>;
			lua_CFunction SetImg = lua_SetObjectVar<float, ObjectVarToFloat, SetImgForObject<Bullet>, SetImgForObject<Enemy>, SetImgForObject<Player>, SetImgForObject<Boss>>;
			lua_CFunction SetAngle = lua_SetObjectVar<float, ObjectVarToFloat, nullptr, SetAngleForObject<Enemy>, nullptr, nullptr>;

			lua_register(L, "SetX", SetX);
			lua_register(L, "SetY", SetY);
			lua_register(L, "SetSpd", SetSpd);
			lua_register(L, "SetDir", SetDir);
			lua_register(L, "SetAcc", SetAcc);
			lua_register(L, "SetSpr", SetSpr);
			lua_register(L, "SetImg", SetImg);
			lua_register(L, "SetAngle", SetAngle);

			lua_pushboolean(L, game.skip_to_midboss);
			lua_setglobal(L, "SKIP_TO_MIDBOSS");
			lua_pushboolean(L, game.skip_to_boss);
			lua_setglobal(L, "SKIP_TO_BOSS");
		}

		{
			auto& scripts = game.assets.GetScripts();

			for (auto it = scripts.begin(); it != scripts.end(); ++it) {
				ScriptData* script = it->second;

				if (!(script->stage_index == game.stage_index || script->stage_index == -1)) {
					continue;
				}

				const char* buffer = script->buffer.data();
				size_t buffer_size = script->buffer.size();
				const char* debug_name = it->first.c_str();

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
		}

		{
			StageData* data = GetStageData(game.stage_index);

			lua_getglobal(L, data->script);
			coroutine = CreateCoroutine(L, L);
		}
	}

	bool CallLuaFunction(lua_State* L, int ref, instance_id id) {
		if (ref == LUA_REFNIL) {
			return true;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
		if (!lua_isfunction(L, -1)) {
			TH_LOG_ERROR("CallLuaFunction: not a function");
			lua_pop(L, 1);
			return false;
		}

		lua_pushinteger(L, id);
		int res = lua_pcall(L, 1, 0, 0);
		if (res != LUA_OK) {
			TH_LOG_ERROR("CallLuaFunction:\n%s", lua_tostring(L, -1));
			lua_pop(L, 1);
			return false;
		}

		return true;
	}

	void UpdateCoroutine(lua_State* L, int* coroutine, instance_id id) {
		if (*coroutine == LUA_REFNIL) {
			return;
		}

		lua_rawgeti(L, LUA_REGISTRYINDEX, *coroutine);
		if (!lua_isthread(L, -1)) {
			TH_SHOW_ERROR("UpdateCoroutine: not a thread");
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
			TH_SHOW_ERROR("UpdateCoroutine:\n%s", lua_tostring(NL, -1));
			lua_settop(NL, 0);
			luaL_unref(L, LUA_REGISTRYINDEX, *coroutine);
			*coroutine = LUA_REFNIL;
		}
	}

}
