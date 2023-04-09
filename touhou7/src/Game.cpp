#include "Game.h"

#include "common.h"
#include "cpml.h"
#include "external/stb_sprintf.h"

#include <SDL_image.h>

namespace th {

	Game* Game::_instance = nullptr;

	bool Game::Init() {
		if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0) {
			TH_SHOW_ERROR("SDL_Init failed : %s", SDL_GetError());
			return false;
		}

		if (!(window = SDL_CreateWindow("touhou7", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GAME_W, GAME_H, SDL_WINDOW_SHOWN))) {
			TH_SHOW_ERROR("SDL_CreateWindow failed : %s", SDL_GetError());
			return false;
		}

		if (!(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE))) {
			TH_SHOW_ERROR("SDL_CreateRenderer failed : %s", SDL_GetError());
			return false;
		}

		if (!(game_surface = SDL_CreateTexture(renderer, TH_SURFACE_FORMAT, SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H))) {
			TH_SHOW_ERROR("couldn't create game surface : %s", SDL_GetError());
			return false;
		}

		SDL_RenderSetLogicalSize(renderer, 0, 0);

		up_surface = SDL_CreateTexture(renderer, TH_SURFACE_FORMAT, SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H);
		SDL_SetTextureScaleMode(up_surface, SDL_ScaleModeLinear);

		if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
			TH_SHOW_ERROR("IMG_Init failed : %s", IMG_GetError());
			return false;
		}

		if (!(Mix_Init(MIX_INIT_MP3) & MIX_INIT_MP3)) {
			TH_SHOW_ERROR("Mix_Init failed : %s", Mix_GetError());
			return false;
		}

		Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048);
		Mix_MasterVolume((int)(0.00f * (float)MIX_MAX_VOLUME));

		TTF_Init();

		assets.LoadAssets(renderer);

		FillDataTables();

		next_scene = TITLE_SCENE;

		SetWindowMode(0);

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

		TTF_Quit();

		Mix_CloseAudio();
		Mix_Quit();

		IMG_Quit();

		SDL_DestroyTexture(up_surface);
		SDL_DestroyTexture(game_surface);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);

		SDL_Quit();
	}

	bool Game::Run() {
		double prev_time = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();

		for (bool running = true; running;) {
			double frame_end_time = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency() + (1.0 / 60.0);

			memset(&key_pressed, 0, sizeof(key_pressed));

			skip_frame = frame_advance;

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

						switch (scancode) {
							case SDL_SCANCODE_F4: {
								if (ev.key.keysym.mod == 0) {
									SetWindowMode(window_mode + 1);
								}
								break;
							}
#ifdef TH_DEBUG
							case SDL_SCANCODE_F2: {
								running = false;
								restart = true;
								break;
							}

							case SDL_SCANCODE_F5: {
								frame_advance = true;
								skip_frame = false;
								break;
							}

							case SDL_SCANCODE_F6: {
								frame_advance = false;
								break;
							}
#endif
						}
						break;
					}
				}
			}

			float delta = 1.0f;

			Update(delta);

			Draw(delta);

			double current_time = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
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

			random.seed();

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

		{
			char buf[11];
			stbsp_snprintf(buf, 11, "%7.2ffps", fps);
			DrawTextBitmap(renderer, assets.fntMain, buf, 30 * 16, 29 * 16);
		}

		int window_w;
		int window_h;
		SDL_GetWindowSize(window, &window_w, &window_h);
		int scale = std::min(window_w / GAME_W, window_h / GAME_H);
		int up_w = GAME_W * scale;
		int up_h = GAME_H * scale;
		int current_w;
		int current_h;
		SDL_QueryTexture(up_surface, nullptr, nullptr, &current_w, &current_h);
		if (current_w != up_w || current_h != up_h) {
			SDL_DestroyTexture(up_surface);
			up_surface = SDL_CreateTexture(renderer, TH_SURFACE_FORMAT, SDL_TEXTUREACCESS_TARGET, up_w, up_h);
			SDL_SetTextureScaleMode(up_surface, SDL_ScaleModeLinear);
		}

		SDL_SetRenderTarget(renderer, up_surface);
		SDL_RenderCopy(renderer, game_surface, nullptr, nullptr);

		SDL_SetRenderTarget(renderer, nullptr);
		SDL_RenderCopy(renderer, up_surface, nullptr, nullptr);

		SDL_RenderPresent(renderer);
	}

	void Game::SetWindowMode(int mode) {
		SDL_DisplayMode desktop;
		SDL_GetDesktopDisplayMode(0, &desktop);
		int max_scale = std::min((desktop.w - 1) / GAME_W, (desktop.h - 1) / GAME_H);
		window_mode = cpml::wrap(mode, max_scale + 1);
		if (window_mode == max_scale) {
			SDL_SetWindowDisplayMode(window, &desktop);
			//SDL_DisplayMode m{0, 640, 480, 60};
			//SDL_SetWindowDisplayMode(window, &m);
			SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
			SDL_ShowCursor(SDL_DISABLE);
		} else {
			SDL_SetWindowSize(window, GAME_W * (window_mode + 1), GAME_H * (window_mode + 1));
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			SDL_SetWindowFullscreen(window, 0);
			SDL_ShowCursor(SDL_ENABLE);
		}
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
