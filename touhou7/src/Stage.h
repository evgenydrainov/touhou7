#pragma once

#include "Objects.h"

#include "xorshf96.h"

#define PLAY_AREA_W 384
#define PLAY_AREA_H 448

#define BOSS_STARTING_X ((float)PLAY_AREA_W / 2.0f)
#define BOSS_STARTING_Y 96.0f

#define PLAYER_STARTING_X ((float)PLAY_AREA_W / 2.0f)
#define PLAYER_STARTING_Y 384.0f

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
		void FreeBoss();

		void StartBossPhase();
		bool EndBossPhase();

		void ScreenShake(float power, float time) {
			screen_shake_power = power;
			screen_shake_timer = time;
			screen_shake_time = time;
		}

		Player player{};
		bool boss_exists = false;
		Boss boss{};
		std::vector<Enemy> enemies;
		std::vector<Bullet> bullets;
		std::vector<Pickup> pickups;
		std::vector<PlayerBullet> player_bullets;

		xorshf96 random;
		lua_State* L = nullptr;
		float time = 0.0f;
		float screen_shake_power = 0.0f;
		float screen_shake_timer = 0.0f;
		float screen_shake_time = 0.0f;
		float screen_shake_x = 0.0f;
		float screen_shake_y = 0.0f;

	private:
		Game& game;
		GameScene& scene;

		void InitLua();
		void PhysicsUpdate(float delta);
		void CallCoroutines();
		void UpdateBoss(float delta);
		void UpdateSpriteComponent(SpriteComponent& sc, float delta);
		void UpdatePlayer(float delta);

		template <typename Object>
		void DrawObject(SDL_Renderer* renderer, Object& object, float angle = 0.0f, float xscale = 1.0f, float yscale = 1.0f, SDL_Color color = {255, 255, 255, 255});

		instance_id next_id = 0;

		int coroutine = LUA_REFNIL;
		float coro_update_timer = 0.0f;
		float spellcard_bg_alpha = 0.0f;

		unsigned char* stage_memory = nullptr;

		friend class GameScene;
	};

}
