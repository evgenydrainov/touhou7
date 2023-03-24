#include "Game.h"

#include "common.h"

#include <SDL_image.h>

#include <iostream>

namespace th
{
	StageData stage_data[STAGE_COUNT]{};
	BulletData bullet_data[BULLET_TYPE_COUNT]{};
	CharacterData char_data[CHARACTER_COUNT]{};

	Game* Game::_instance = nullptr;

	bool Game::Init()
	{
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

		scene.Init();

		//SDL_DisplayMode m;
		//m.w = 1920;
		//m.h = 1080;
		//m.refresh_rate = 75;
		//SDL_SetWindowDisplayMode(window, &m);
		//SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

		return true;
	}

	void Game::Shutdown()
	{
		scene.Quit();

		assets.UnloadAssets();

		IMG_Quit();

		SDL_DestroyTexture(game_surface);

		SDL_DestroyRenderer(renderer);

		SDL_DestroyWindow(window);

		SDL_Quit();
	}

	bool Game::Run()
	{
		double prev_time = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();

		for (bool running = true; running;) {
			double frame_end_time = (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency() + (1.0 / 60.0);

			//std::cout.precision(17);
			//std::cout << std::fixed << frame_end_time << std::endl;
			//std::cout << SDL_GetPerformanceCounter() << std::endl;
			//std::cout << SDL_GetPerformanceFrequency() << std::endl;

			for (SDL_Event ev; SDL_PollEvent(&ev);) {
				switch (ev.type) {
					case SDL_QUIT: {
						running = false;
						break;
					}
					case SDL_KEYDOWN: {
						if (ev.key.keysym.scancode == SDL_SCANCODE_F2) {
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

	void Game::Update(float delta)
	{
		scene.Update(delta);
	}

	void Game::Draw(float delta)
	{
		scene.Draw(renderer, game_surface, delta);

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

	void Game::FillDataTables()
	{
		char_data[CHARACTER_REIMU].name           = "Reimu Hakurei";
		char_data[CHARACTER_REIMU].move_spd       = 3.75f;
		char_data[CHARACTER_REIMU].focus_spd      = 1.6f;
		char_data[CHARACTER_REIMU].radius         = 2.0f;
		char_data[CHARACTER_REIMU].graze_radius   = 16.0f;
		char_data[CHARACTER_REIMU].deathbomb_time = 15.0f;
		char_data[CHARACTER_REIMU].starting_bombs = 2;
		//char_data[CHARACTER_REIMU].shot_type      = reimu_shot_type;
		//char_data[CHARACTER_REIMU].bomb           = reimu_bomb;
		char_data[CHARACTER_REIMU].idle_spr       = assets.GetSprite("ReimuIdle");
		char_data[CHARACTER_REIMU].turn_spr       = assets.GetSprite("ReimuTurn");

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

		stage_data[0].script = "Stage1_Script";

		stage_data[0].boss_data[1].name        = "Daiyousei";
		stage_data[0].boss_data[1].phase_count = 1;
		stage_data[0].boss_data[1].sprite      = assets.GetSprite("Daiyousei");
		stage_data[0].boss_data[1].type        = BOSS_MIDBOSS;

		stage_data[0].boss_data[1].phase_data[0].hp     = 1000.f;
		stage_data[0].boss_data[1].phase_data[0].time   = 31.f * 60.f;
		stage_data[0].boss_data[1].phase_data[0].type   = PHASE_NONSPELL;
		stage_data[0].boss_data[1].phase_data[0].script = "Daiyousei_Nonspell1";
	}
}
