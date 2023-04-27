#include "Game.h"

#include "common.h"
#include "cpml.h"
#include "external/stb_sprintf.h"

#include <SDL_image.h>

#define SDL_CheckError(expr) \
	if (!(expr)) TH_SHOW_ERROR("%s", SDL_GetError())
#define SDL_CheckErrorMsg(expr, msg) \
	if (!(expr)) TH_SHOW_ERROR(msg ": %s", SDL_GetError())

namespace th {

	Game* Game::_instance = nullptr;

	bool Game::Init() {
		SDL_CheckErrorMsg(SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) == 0, "couldn't initialize SDL");

		// @hack
		// behaves differently on different render backends
		SDL_CheckError(SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "1"));

		SDL_CheckError(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"));
		SDL_CheckError(SDL_SetHint(SDL_HINT_RENDER_BATCHING, "1"));
		SDL_CheckError(SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"));
		SDL_CheckError(SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0"));

		//SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "0");
		//SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

		SDL_CheckError(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2) == 0);
		SDL_CheckError(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1) == 0);

		SDL_CheckErrorMsg(window = SDL_CreateWindow("touhou7", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, GAME_W, GAME_H, SDL_WINDOW_SHOWN), "couldn't create the window");
		SDL_CheckErrorMsg(renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE), "couldn't create the renderer");
		SDL_CheckErrorMsg(game_surface = SDL_CreateTexture(renderer, TH_SURFACE_FORMAT, SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H), "couldn't create game surface");
		SDL_CheckErrorMsg(up_surface = SDL_CreateTexture(renderer, TH_SURFACE_FORMAT, SDL_TEXTUREACCESS_TARGET, GAME_W, GAME_H), "couldn't create up surface");

		SDL_CheckError(SDL_SetTextureScaleMode(up_surface, SDL_ScaleModeLinear) == 0);

		{
			SDL_RendererInfo info;
			SDL_CheckError(SDL_GetRendererInfo(renderer, &info) == 0);
			printf("render backend: %s\n", info.name);
		}

		{
			int major;
			int minor;
			SDL_CheckError(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major) == 0);
			SDL_CheckError(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor) == 0);
			printf("opengl major version: %d\n", major);
			printf("opengl minor version: %d\n", minor);
		}

		//SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		//SDL_RenderClear(renderer);
		//SDL_RenderPresent(renderer);

		SDL_CheckErrorMsg(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG, "couldn't initialize SDL_image");
		SDL_CheckErrorMsg(Mix_Init(MIX_INIT_MP3) & MIX_INIT_MP3, "couldn't initialize SDL_mixer");

		SDL_CheckError(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, MIX_DEFAULT_CHANNELS, 2048) == 0);
		Mix_MasterVolume((int)(0.25f * (float)MIX_MAX_VOLUME));

		SDL_CheckErrorMsg(TTF_Init() == 0, "couldn't initialize SDL_ttf");

		assets.LoadAssets(renderer);

		FillDataTables();

		next_scene = TITLE_SCENE;

		SetWindowMode(0);

		//printf("done\n");

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

	static double GetTime() {
		return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
	}

	bool Game::Run() {
		double prev_time = GetTime();

		for (bool running = true; running;) {
			double frame_end_time = GetTime() + (1.0 / 60.0);

			memset(&key_pressed, 0, sizeof(key_pressed));

			skip_frame = frame_advance;

			for (SDL_Event ev; SDL_PollEvent(&ev);) {
				switch (ev.type) {
					case SDL_QUIT: {
						running = false;
						break;
					}

					case SDL_KEYDOWN: {
						int scancode = ev.key.keysym.scancode;

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
						}

						if (debug) {
							switch (scancode) {
								case SDL_SCANCODE_F1: {
									show_debug ^= true;
									break;
								}
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
							}
						}
						break;
					}
				}
			}

			float delta = 1.0f;

			Update(delta);

			Draw(delta);

			double current_time = GetTime();
			frame_took = current_time - prev_time;
			fps = 1.0 / (current_time - prev_time);
			prev_time = current_time;

#if 1
			double time_left = frame_end_time - current_time;

			if (time_left > 0.0) {
				double time_to_sleep = time_left * 0.95;
				//double time_to_sleep = time_left;
				SDL_Delay((unsigned int)(time_to_sleep * 1000.0));

				//SDL_Delay(std::max((int)(time_left * 1000.0) - 1, 0));

				while (GetTime() < frame_end_time) {}
			}
#endif
		}

		return true;
	}

	void Game::Update(float delta) {
		double update_start_t = GetTime();
		everything_start_t = GetTime();

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

		double update_end_t = GetTime();
		update_took = update_end_t - update_start_t;
	}

	void Game::Draw(float delta) {
		double draw_start_t = GetTime();

#if 1
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
			stbsp_snprintf(buf, sizeof(buf), "%7.2ffps", fps);
			DrawTextBitmap(renderer, assets.fntMain, buf, 30 * 16, 29 * 16);
		}

		if (show_debug) {
			char buf[100];
			stbsp_snprintf(
				buf,
				sizeof(buf),
				"SDL allocs: %d\n"
				"update %.2fms\n"
				"draw %.2fms\n"
				"everything %.2fms\n"
				"frame %.2fms",
				(int)SDL_GetNumAllocations(),
				1000.0 * update_took,
				1000.0 * draw_took,
				1000.0 * everything_took,
				1000.0 * frame_took
			);
			int x = 0;
			int y = 0;
			SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(assets.fntCirno, buf, {255, 255, 255, 255}, 0);
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			{
				SDL_SetTextureColorMod(texture, 0, 0, 0);
				SDL_Rect dest{x + 1, y + 1, surface->w, surface->h};
				SDL_RenderCopy(renderer, texture, nullptr, &dest);
			}
			{
				SDL_SetTextureColorMod(texture, 255, 64, 64);
				SDL_Rect dest{x, y, surface->w, surface->h};
				SDL_RenderCopy(renderer, texture, nullptr, &dest);
			}
			SDL_DestroyTexture(texture);
			SDL_FreeSurface(surface);
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
#endif

		//stage_data[0].draw(this, renderer, 0, true, 1.0f);

		double draw_end_t = GetTime();
		draw_took = draw_end_t - draw_start_t;
		double everything_end_t = GetTime();
		everything_took = everything_end_t - everything_start_t;

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
			fullscreen = true;
		} else {
			SDL_SetWindowSize(window, GAME_W * (window_mode + 1), GAME_H * (window_mode + 1));
			SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
			SDL_SetWindowFullscreen(window, 0);
			SDL_ShowCursor(SDL_ENABLE);
			fullscreen = false;
		}
	}

}
