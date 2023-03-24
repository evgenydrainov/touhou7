#pragma once

#include "Assets.h"

#include "xorshf96.h"

#include <lua.hpp>

#define PLAY_AREA_W 384
#define PLAY_AREA_H 448

#define TYPE_PART_SHIFT 28
#define ID_PART_MASK 0x0FFF'FFFF

namespace th
{
	class Game;

	class GameScene;

	struct SpriteComponent
	{
		SpriteData* sprite;
		float frame_index;
		float angle;
		float xscale = 1.0f;
		float yscale = 1.0f;
	};

	enum class PlayerState : unsigned char {
		Normal,
		Dying,
		Appearing
	};

	struct Player
	{
		unsigned int id;
		bool dead;
		float x;
		float y;
		float hsp;
		float vsp;
		float radius;
		SpriteComponent sc;
		PlayerState state;
		bool focus;
		float iframes;
		float bomb_timer;
		float hitbox_alpha;
	};

	struct Bullet
	{
		unsigned int id;
		bool dead;
		float x;
		float y;
		float spd;
		float dir;
		float acc;
		float radius;
		SpriteComponent sc;
		float lifetime;
		bool rotate;
		int coroutine = LUA_REFNIL;
		int update_callback = LUA_REFNIL;
	};

	struct Enemy
	{
		unsigned int id;
		bool dead;
		float x;
		float y;
		float spd;
		float dir;
		float acc;
		SpriteComponent sc;
		unsigned int drops;
		int coroutine = LUA_REFNIL;
		int update_callback = LUA_REFNIL;
		int death_callback = LUA_REFNIL;
	};

	enum class BossState : unsigned char
	{
		Normal,
		WaitingStart,
		WaitingEnd
	};

	struct Boss
	{
		unsigned int id;
		bool dead;
		float x;
		float y;
		float spd;
		float dir;
		float acc;
		float radius;
		SpriteComponent sc;
		float hp;
		int type_index;
		int phase_index;
		float timer;
		float wait_timer;
		BossState state;
		int coroutine = LUA_REFNIL;
	};

	class Stage
	{
	public:
		Stage(Game& game, GameScene& scene) : game(game), scene(scene) {}

		bool Init();
		void Quit();

		void Update(float delta);
		void Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta);

		Enemy& CreateEnemy();
		Bullet& CreateBullet();
		Player& CreatePlayer();
		Boss& CreateBoss();

		Enemy* FindEnemy(unsigned int id);
		Bullet* FindBullet(unsigned int id);
		Player* FindPlayer(unsigned int id);
		Boss* FindBoss(unsigned int id);

		void FreeEnemy(Enemy& enemy);
		void FreeBullet(Bullet& bullet);
		void FreePlayer(Player& player);
		void FreeBoss(Boss& boss);

		float random_range(float a, float b);

		void BossStartPhase(Boss& boss);
		bool BossEndPhase(Boss& boss);

		Player player_storage{};
		bool boss_exists = false;
		Boss boss_storage{};
		std::vector<Enemy> enemies;
		std::vector<Bullet> bullets;

		xorshf96 rEngine;
		lua_State* L = nullptr;
		float time = 0.0f;

	private:
		Game& game;
		GameScene& scene;

		void PhysicsUpdate(float delta);
		void CallCoroutines();
		void BossUpdate(Boss& boss, float delta);
		void UpdateSpriteComponent(SpriteComponent& sc, float delta);

		unsigned int next_id = 0;

		int coroutine = LUA_REFNIL;

		float coro_update_timer = 0.0f;
	};
}
