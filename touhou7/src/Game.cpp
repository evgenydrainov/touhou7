#include "Game.h"

#include "common.h"
#include "cpml.h"

#include <SDL_image.h>

#include <iostream>

namespace th {

	StageData stage_data[STAGE_COUNT]{};
	BulletData bullet_data[BULLET_TYPE_COUNT]{};
	CharacterData character_data[CHARACTER_COUNT]{};
	BossData boss_data[BOSS_TYPE_COUNT]{};

	Game* Game::_instance = nullptr;

	bool Game::Init() {
		if (SDL_Init(SDL_INIT_VIDEO) != 0) {
			TH_SHOW_ERROR("SDL_Init failed : %s", SDL_GetError());
			return false;
		}

		if (!(window = SDL_CreateWindow("touhou7", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GAME_W, GAME_H, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE))) {
			TH_SHOW_ERROR("SDL_CreateWindow failed : %s", SDL_GetError());
			return false;
		}

		if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE))) {
			TH_SHOW_ERROR("SDL_CreateRenderer failed : %s", SDL_GetError());
			return false;
		}

		SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);

		if (!(game_surface = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H))) {
			TH_SHOW_ERROR("couldn't create game surface : %s", SDL_GetError());
			return false;
		}

		if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
			TH_SHOW_ERROR("IMG_Init failed : %s", IMG_GetError());
			return false;
		}

		assets.LoadAssets();

		FillDataTables();

		next_scene = TITLE_SCENE;

		//SDL_DisplayMode m;
		//m.w = 1920;
		//m.h = 1080;
		//m.refresh_rate = 75;
		//SDL_SetWindowDisplayMode(window, &m);
		//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

		return true;
	}

	void Game::Shutdown() {
		static_assert(LAST_SCENE == 3);
		switch (scene.index()) {
			case GAME_SCENE: {
				std::get<GAME_SCENE>(scene).Quit();
				break;
			}
			case TITLE_SCENE: {
				std::get<TITLE_SCENE>(scene).Quit();
				break;
			}
		}

		assets.UnloadAssets();

		IMG_Quit();

		SDL_DestroyTexture(game_surface);

		SDL_DestroyRenderer(renderer);

		SDL_DestroyWindow(window);

		SDL_Quit();
	}

	bool Game::Run() {
		double prev_time = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();

		for (bool running = true; running;) {
			double frame_end_time = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency() + (1.0 / 60.0);

			//std::cout.precision(17);
			//std::cout << std::fixed << frame_end_time << std::endl;
			//std::cout << SDL_GetPerformanceCounter() << std::endl;
			//std::cout << SDL_GetPerformanceFrequency() << std::endl;

			memset(&key_pressed, 0, sizeof(key_pressed));

			for (SDL_Event ev; SDL_PollEvent(&ev);) {
				switch (ev.type) {
					case SDL_QUIT: {
						running = false;
						break;
					}
					case SDL_KEYDOWN: {
						SDL_Scancode scancode = ev.key.keysym.scancode;

						if (0 <= scancode && scancode < sizeof(key_pressed)) {
							key_pressed[scancode] = true;
						}

						if (scancode == SDL_SCANCODE_F2) {
							running = false;
							restart = true;
						}
						break;
					}
				}
			}

			float delta = 1.0f;

			Update(delta);

			Draw(delta);

			double current_time = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
			//std::cout << 1.0 / (current_time - prev_time) << std::endl;
			fps = 1.0 / (current_time - prev_time);
			prev_time = current_time;

			double time_left = frame_end_time - current_time;

			if (time_left > 0.0) {
				double time_to_sleep = time_left * 0.95;
				//double time_to_sleep = time_left;

				//*
				SDL_Delay((unsigned int)(time_to_sleep * 1000.0));

				while ((double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency() < frame_end_time);
				//*/
			}
		}

		return true;
	}

	void Game::Update(float delta) {
		static_assert(LAST_SCENE == 3);
		switch (scene.index()) {
			case GAME_SCENE: {
				std::get<GAME_SCENE>(scene).Update(delta);
				break;
			}
			case TITLE_SCENE: {
				std::get<TITLE_SCENE>(scene).Update(delta);
				break;
			}
		}

		if (next_scene != 0) {
			static_assert(LAST_SCENE == 3);
			switch (scene.index()) {
				case GAME_SCENE: {
					std::get<GAME_SCENE>(scene).Quit();
					break;
				}
				case TITLE_SCENE: {
					std::get<TITLE_SCENE>(scene).Quit();
					break;
				}
			}

			int s = next_scene;
			next_scene = 0;

			static_assert(LAST_SCENE == 3);
			switch (s) {
				case GAME_SCENE: {
					game_scene = &scene.emplace<GAME_SCENE>(*this);
					game_scene->Init();
					break;
				}
				case TITLE_SCENE: {
					title_scene = &scene.emplace<TITLE_SCENE>(*this);
					title_scene->Init();
					break;
				}
			}
		}
	}

	void Game::Draw(float delta) {
		static_assert(LAST_SCENE == 3);
		switch (scene.index()) {
			case GAME_SCENE: {
				std::get<GAME_SCENE>(scene).Draw(renderer, game_surface, delta);
				break;
			}
			case TITLE_SCENE: {
				std::get<TITLE_SCENE>(scene).Draw(renderer, game_surface, delta);
				break;
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		SDL_RenderCopy(renderer, game_surface, nullptr, nullptr);

		{
			char buf[10];
			snprintf(buf, 10, "%6.2ffps", fps);
			DrawText(renderer, &assets.fntMain, buf, 31 * 16, 29 * 16);
		}

		SDL_RenderPresent(renderer);
	}

	void ReimuShotType(Game* ctx, float delta);
	void ReimuBomb(Game* ctx);

	void Game::FillDataTables() {
		static_assert(CHARACTER_COUNT == 1);

		character_data[CHARACTER_REIMU].name           = "Reimu Hakurei";
		character_data[CHARACTER_REIMU].move_spd       = 3.75f;
		character_data[CHARACTER_REIMU].focus_spd      = 1.6f;
		character_data[CHARACTER_REIMU].radius         = 2.0f;
		character_data[CHARACTER_REIMU].graze_radius   = 16.0f;
		character_data[CHARACTER_REIMU].deathbomb_time = 15.0f;
		character_data[CHARACTER_REIMU].starting_bombs = 2;
		character_data[CHARACTER_REIMU].shot_type      = ReimuShotType;
		character_data[CHARACTER_REIMU].bomb           = ReimuBomb;
		character_data[CHARACTER_REIMU].idle_spr       = assets.GetSprite("ReimuIdle");
		character_data[CHARACTER_REIMU].turn_spr       = assets.GetSprite("ReimuTurn");

		static_assert(BULLET_TYPE_COUNT == 7);

		bullet_data[0].sprite = assets.GetSprite("Bullet0");
		bullet_data[1].sprite = assets.GetSprite("Bullet1");
		bullet_data[2].sprite = assets.GetSprite("Bullet2");
		bullet_data[3].sprite = assets.GetSprite("Bullet3");
		bullet_data[4].sprite = assets.GetSprite("Bullet4");
		bullet_data[5].sprite = assets.GetSprite("Bullet5");
		bullet_data[6].sprite = assets.GetSprite("Bullet6");

		bullet_data[0].radius = 3.0f;
		bullet_data[1].radius = 3.0f;
		bullet_data[2].radius = 4.0f;
		bullet_data[3].radius = 2.0f;
		bullet_data[4].radius = 2.0f;
		bullet_data[5].radius = 2.0f;
		bullet_data[6].radius = 2.0f;

		bullet_data[0].rotate = true;
		bullet_data[1].rotate = false;
		bullet_data[2].rotate = false;
		bullet_data[3].rotate = true;
		bullet_data[4].rotate = true;
		bullet_data[5].rotate = true;
		bullet_data[6].rotate = false;

		static_assert(STAGE_COUNT == 1);

		stage_data[0].script = "Stage1_Script";

		static_assert(BOSS_TYPE_COUNT == 2);

		boss_data[0].name        = "Daiyousei";
		boss_data[0].phase_count = 1;
		boss_data[0].sprite      = assets.GetSprite("Daiyousei");
		boss_data[0].type        = BOSS_MIDBOSS;

		boss_data[0].phase_data[0].hp     = 1000.0f;
		boss_data[0].phase_data[0].time   = 31.0f * 60.0f;
		boss_data[0].phase_data[0].type   = PHASE_NONSPELL;
		boss_data[0].phase_data[0].script = "Daiyousei_Nonspell1";

		boss_data[1].name = "Cirno";
		boss_data[1].phase_count = 5;
		boss_data[1].sprite = assets.GetSprite("CirnoIdle");
		boss_data[1].type = BOSS_BOSS;

		boss_data[1].phase_data[0].hp     = 1500.0f;
		boss_data[1].phase_data[0].time   = 25.0f * 60.0f;
		boss_data[1].phase_data[0].type   = PHASE_NONSPELL;
		boss_data[1].phase_data[0].script = "Cirno_Nonspell1";

		boss_data[1].phase_data[1].hp     = 1500.0f;
		boss_data[1].phase_data[1].time   = 30.0f * 60.0f;
		boss_data[1].phase_data[1].type   = PHASE_SPELLCARD;
		boss_data[1].phase_data[1].script = "Cirno_IcicleFall";
		boss_data[1].phase_data[1].name   = "Ice Sign \"Icicle Fall\"";

		boss_data[1].phase_data[2].hp     = 1500.0f;
		boss_data[1].phase_data[2].time   = 50.0f * 60.0f;
		boss_data[1].phase_data[2].type   = PHASE_NONSPELL;
		boss_data[1].phase_data[2].script = "Cirno_Nonspell2";

		boss_data[1].phase_data[3].hp     = 1500.0f;
		boss_data[1].phase_data[3].time   = 40.0f * 60.0f;
		boss_data[1].phase_data[3].type   = PHASE_SPELLCARD;
		boss_data[1].phase_data[3].script = "Cirno_PerfectFreeze";
		boss_data[1].phase_data[3].name   = "Freeze Sign \"Perfect Freeze\"";

		boss_data[1].phase_data[4].hp     = 1500.0f;
		boss_data[1].phase_data[4].time   = 33.0f * 60.0f;
		boss_data[1].phase_data[4].type   = PHASE_SPELLCARD;
		boss_data[1].phase_data[4].script = "Cirno_DiamondBlizzard";
		boss_data[1].phase_data[4].name   = "Snow Sign \"Diamond Blizzard\"";
	}

	StageData* GetStageData(int stage_index) {
		StageData* result;
		if (0 <= stage_index && stage_index < STAGE_COUNT) {
			result = &stage_data[stage_index];
		} else {
			result = &stage_data[0];
		}
		return result;
	}

	BulletData* GetBulletData(int type_index) {
		BulletData* result;
		if (0 <= type_index && type_index < BULLET_TYPE_COUNT) {
			result = &bullet_data[type_index];
		} else {
			result = &bullet_data[0];
		}
		return result;
	}

	CharacterData* GetCharacterData(int character_index) {
		CharacterData* result;
		if (0 <= character_index && character_index < CHARACTER_COUNT) {
			result = &character_data[character_index];
		} else {
			result = &character_data[0];
		}
		return result;
	}

	BossData* GetBossData(int type_index) {
		BossData* result;
		if (0 <= type_index && type_index < BOSS_TYPE_COUNT) {
			result = &boss_data[type_index];
		} else {
			result = &boss_data[0];
		}
		return result;
	}

	PhaseData* GetPhaseData(BossData* boss_data, int phase_index) {
		PhaseData* result;
		if (!boss_data) {
			boss_data = GetBossData(0);
		}
		if (0 <= phase_index && phase_index < boss_data->phase_count) {
			result = &boss_data->phase_data[phase_index];
		} else {
			result = &boss_data->phase_data[0];
		}
		return result;
	}

}
