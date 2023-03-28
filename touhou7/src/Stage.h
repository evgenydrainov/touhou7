#pragma once

#include "Objects.h"

#include "xorshf96.h"

#define PLAY_AREA_W 384
#define PLAY_AREA_H 448

namespace th {

	class Game;

	class GameScene;

	class Stage {
	public:
		Stage(Game& game, GameScene& scene) : game(game), scene(scene) {}

		bool Init();
		void Quit();

		void Update(float delta);
		void Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta);

		Enemy& CreateEnemy();
		Bullet& CreateBullet();
		Player& CreatePlayer(bool from_death = false);
		Boss& CreateBoss();
		Pickup& CreatePickup(float x, float y, unsigned char type);
		PlayerBullet& CreatePlayerBullet();

		Enemy* FindEnemy(instance_id id);
		Bullet* FindBullet(instance_id id);
		Player* FindPlayer(instance_id id);
		Boss* FindBoss(instance_id id);

		void FreeEnemy(Enemy& enemy);
		void FreeBullet(Bullet& bullet);
		void FreePlayer();
		void FreeBoss();

		float random_range(float a, float b);

		void StartBossPhase();
		bool EndBossPhase();

		Player player{};
		bool boss_exists = false;
		Boss boss{};
		std::vector<Enemy> enemies;
		std::vector<Bullet> bullets;
		std::vector<Pickup> pickups;
		std::vector<PlayerBullet> player_bullets;

		xorshf96 rEngine;
		lua_State* L = nullptr;
		float time = 0.0f;

	private:
		Game& game;
		GameScene& scene;

		void PhysicsUpdate(float delta);
		void CallCoroutines();
		void UpdateBoss(float delta);
		void UpdateSpriteComponent(SpriteComponent& sc, float delta);
		void UpdatePlayer(float delta);

		instance_id next_id = 0;

		int coroutine = LUA_REFNIL;

		float coro_update_timer = 0.0f;
	};

}
