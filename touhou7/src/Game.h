#pragma once

#include "Assets.h"
#include "GameScene.h"
#include "TitleScene.h"

#include <SDL.h>

#include <variant>

#define GAME_W 640
#define GAME_H 480

#define MAX_POWER 128

namespace th {

	enum {
		GAME_SCENE = 1,
		TITLE_SCENE,

		LAST_SCENE
	};

	struct Options {
		int starting_lives = 2;
	};

	class Game {
	public:
		Game() : assets(*this) { _instance = this; }

		static Game& GetInstance() { return *_instance; }

		bool Init();
		void Shutdown();
		bool Run();

		void Update(float delta);
		void Draw(float delta);

		void GoToScene(int scene) { next_scene = scene; }

		Options options{};
		SDL_Renderer* renderer = nullptr;
		Assets assets;

		static_assert(LAST_SCENE == 3);
		std::variant<
			std::monostate,
			GameScene,
			TitleScene
		> scene;

		static_assert(LAST_SCENE == 3);
		union {
			GameScene* game_scene = nullptr;
			TitleScene* title_scene;
		};

		int player_character = 0;

		bool restart = false;

		bool skip_to_midboss = false;
		bool skip_to_boss = false;

		bool key_pressed[30]{};

	private:
		static Game* _instance;

		void FillDataTables();

		SDL_Window* window = nullptr;
		SDL_Texture* game_surface = nullptr;

		int next_scene = 0;

		double fps = 0.0;
	};

	enum {
		PHASE_NONSPELL,
		PHASE_SPELLCARD,
	};

	struct PhaseData {
		const char* script;
		float hp;
		float time;
		unsigned char type;
		const char* name;
	};

	enum {
		BOSS_MIDBOSS,
		BOSS_BOSS
	};

	struct BossData {
		const char* name;
		PhaseData phase_data[25];
		int phase_count;
		SpriteData* sprite;
		unsigned char type;
	};

	struct StageData {
		const char* script;
	};

	struct BulletData {
		SpriteData* sprite;
		float radius;
		bool rotate;
	};

	struct CharacterData {
		const char* name;
		float move_spd;
		float focus_spd;
		float radius;
		float graze_radius;
		float deathbomb_time;
		int starting_bombs;
		void (*shot_type)(Game* ctx, float delta);
		void (*bomb)(Game* ctx);
		SpriteData* idle_spr;
		SpriteData* turn_spr;
	};

	enum {
		CHARACTER_REIMU,
		//CHARACTER_MARISA,

		CHARACTER_COUNT
	};

#define STAGE_COUNT 1
#define BULLET_TYPE_COUNT 7
#define BOSS_TYPE_COUNT 2

	extern StageData stage_data[STAGE_COUNT];
	extern BulletData bullet_data[BULLET_TYPE_COUNT];
	extern CharacterData character_data[CHARACTER_COUNT];
	extern BossData boss_data[BOSS_TYPE_COUNT];
	//extern float hiscore_data[CHARACTER_COUNT];

	StageData* GetStageData(int stage_index);
	BulletData* GetBulletData(int type_index);
	CharacterData* GetCharacterData(int character_index);
	BossData* GetBossData(int type_index);
	PhaseData* GetPhaseData(BossData* boss_data, int phase_index);

}
