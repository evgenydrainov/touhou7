#include "GameScene.h"

#include "Game.h"

#include "common.h"
#include "cpml.h"
#include "external/stb_sprintf.h"

namespace th {

	static int GetNextPointLevel(int points) {
		int res;
		if (points >= 800) {
			res = 800 + ((points - 800) / 200 + 1) * 200;
		} else if (points >= 450) {
			res = 800;
		} else if (points >= 300) {
			res = 450;
		} else if (points >= 200) {
			res = 300;
		} else if (points >= 125) {
			res = 200;
		} else if (points >= 50) {
			res = 125;
		} else {
			res = 50;
		}
		return res;
	}

	bool GameScene::Init() {
		if (!(play_area_surface = SDL_CreateTexture(game.renderer, TH_SURFACE_FORMAT, SDL_TEXTUREACCESS_TARGET, PLAY_AREA_W, PLAY_AREA_H))) {
			TH_SHOW_ERROR("couldn't create play area surface : %s", SDL_GetError());
			return false;
		}

		ResetStats();

		stage.emplace(game, *this);

		stage->Init();

		return true;
	}

	void GameScene::Quit() {
		stage->Quit();

		SDL_DestroyTexture(play_area_surface);
	}

	void GameScene::Update(float delta) {
		if (game.key_pressed[SDL_SCANCODE_ESCAPE] || game.key_pressed[SDL_SCANCODE_RETURN]) {
			paused ^= true;
		}

		if (paused) {
			if (game.key_pressed[SDL_SCANCODE_X]) {
				game.GoToScene(TITLE_SCENE);
			}
			if (game.key_pressed[SDL_SCANCODE_Z]) {
				paused = false;
			}
		} else {
			if (!game.skip_frame) {
				stage->Update(delta);
			}
		}

		if (game.debug) {
			if (game.key_pressed[SDL_SCANCODE_P]) GetPower(8);
		}
	}

	void GameScene::ResetStats() {
		stats = {};
		stats.lives = game.options.starting_lives;
		stats.bombs = GetCharacterData(game.player_character)->starting_bombs;
	}

	void GameScene::Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta) {
		if (!paused) {
			stage->Draw(renderer, play_area_surface, delta);
		}

		SDL_SetRenderTarget(renderer, target);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			// bg
			{
				SDL_Texture* texture = game.assets.GetTexture("Background.png");
				SDL_RenderCopy(renderer, texture, nullptr, nullptr);
			}

			// play area
			{
				SDL_Rect dest;
				dest.x = PLAY_AREA_X;
				dest.y = PLAY_AREA_Y;
				dest.w = PLAY_AREA_W;
				dest.h = PLAY_AREA_H;
				if (paused) {
					SDL_SetTextureColorMod(play_area_surface, 128, 128, 128);
				} else {
					SDL_SetTextureColorMod(play_area_surface, 255, 255, 255);
				}
				SDL_RenderCopy(renderer, play_area_surface, nullptr, &dest);

				if (paused) {
					DrawTextBitmap(renderer, game.assets.fntMain, "PAUSED", PLAY_AREA_X + (PLAY_AREA_W - (6 * 15)) / 2, PLAY_AREA_Y + (PLAY_AREA_H - 16) / 2);
				}
			}

			// GUI

			// sidebar
			{
				SpriteData* sprite = game.assets.GetSprite("Sidebar");
				float x = (float)(PLAY_AREA_X + PLAY_AREA_W + 16);
				float y = (float)(PLAY_AREA_Y + 32);
				DrawSprite(renderer, sprite, 0, x, y, 0.0f);
			}

			// STATS
			{
				int x = PLAY_AREA_X + PLAY_AREA_W + 5 * 16;
				int y = PLAY_AREA_Y + 2 * 16;

				// hiscore
				{
					char buf[10];
					stbsp_snprintf(buf, sizeof(buf), "%09d", 0);
					DrawTextBitmap(renderer, game.assets.fntMain, buf, x, y);
				}
				// score
				{
					char buf[10];
					stbsp_snprintf(buf, sizeof(buf), "%09d", stats.score);
					DrawTextBitmap(renderer, game.assets.fntMain, buf, x, y + 16);
				}
				// lives
				{
					SpriteData* sprite = game.assets.GetSprite("UI_Life");
					for (int i = 0; i < stats.lives; i++) {
						int xx = x + i * 16;
						int yy = y + 3 * 16;
						DrawSprite(renderer, sprite, 0, (float)xx, (float)yy);
					}
				}
				// bombs
				{
					SpriteData* sprite = game.assets.GetSprite("UI_Bomb");
					for (int i = 0; i < stats.bombs; i++) {
						int xx = x + i * 16;
						int yy = y + 4 * 16;
						DrawSprite(renderer, sprite, 0, (float)xx, (float)yy);
					}
				}
				// power
				{
					SDL_Rect rect;
					rect.x = x;
					rect.y = y + 6 * 16;
					rect.w = (int)(135.0f * ((float)stats.power / (float)MAX_POWER));
					rect.h = 16;
					SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
					SDL_RenderFillRect(renderer, &rect);

					char buf[4] = "MAX";
					if (stats.power < MAX_POWER) {
						stbsp_snprintf(buf, sizeof(buf), "%d", stats.power);
					}
					DrawTextBitmap(renderer, game.assets.fntMain, buf, rect.x, rect.y);
				}
				// graze
				{
					char buf[10];
					stbsp_snprintf(buf, sizeof(buf), "%d", stats.graze);
					DrawTextBitmap(renderer, game.assets.fntMain, buf, x, y + 7 * 16);
				}
				// points
				{
					char buf[10];
					int next = GetNextPointLevel(stats.points);
					stbsp_snprintf(buf, sizeof(buf), "%d/%d", stats.points, next);
					DrawTextBitmap(renderer, game.assets.fntMain, buf, x, y + 8 * 16);
				}
			}

			// logo
			{
				SDL_Texture* texture = game.assets.GetTexture("Logo.png");
				SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
				SDL_Rect dest;
				dest.x = PLAY_AREA_X + PLAY_AREA_W + 3 * 16;
				dest.y = PLAY_AREA_Y + 15 * 16;
				dest.w = 8 * 16;
				dest.h = 8 * 16;
				SDL_RenderCopy(renderer, texture, nullptr, &dest);
			}

			// bottom enemy label
			if (stage->boss_exists) {
				SpriteData* sprite = game.assets.GetSprite("EnemyLabel");
				float x = (float)PLAY_AREA_X + std::clamp(stage->boss.x, (float)sprite->width / 2.0f, (float)PLAY_AREA_W - (float)sprite->width / 2.0f);
				float y = (float)PLAY_AREA_Y + (float)PLAY_AREA_H;
				DrawSprite(renderer, sprite, 0, x, y);
			}

			// DEBUG
			if (game.show_debug) {
				char buf[200];
				stbsp_snprintf(
					buf,
					sizeof(buf),
					"Next ID %d\n"
					"Lua Mem %fKb\n"
					"Bullets %d (cap %d)\n"
					"Enemies %d (cap %d)\n"
					"Pickups %d (cap %d)\n"
					"PlBullets %d (cap %d)",
					stage->next_id,
					(double)(lua_gc(stage->L, LUA_GCCOUNT) * 1024 + lua_gc(stage->L, LUA_GCCOUNTB)) / 1024.0,
					(int)stage->bullets.size(), (int)stage->bullets.capacity(),
					(int)stage->enemies.size(), (int)stage->enemies.capacity(),
					(int)stage->pickups.size(), (int)stage->pickups.capacity(),
					(int)stage->player_bullets.size(), (int)stage->player_bullets.capacity()
				);
				int x = PLAY_AREA_X + PLAY_AREA_W + 16;
				int y = PLAY_AREA_Y + 11 * 16;
				//DrawTextBitmap(renderer, game.assets.fntMain, buf, x, y);

				SDL_Surface* surface = TTF_RenderText_Blended_Wrapped(game.assets.fntCirno, buf, {255, 255, 255, 255}, 0);
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
		}
	}

	void GameScene::GetScore(int score) {
		stats.score += score;
	}

	void GameScene::GetLives(int lives) {
		while (lives--) {
			if (stats.lives < 8) {
				stats.lives++;

				Mix_Chunk* sound = game.assets.GetSound("se_extend.wav");
				StopSound(sound);
				Mix_PlayChannel(-1, sound, 0);
			} else {
				GetBombs(1);
			}
		}
	}

	void GameScene::GetBombs(int bombs) {
		while (bombs--) {
			if (stats.bombs < 8) {
				stats.bombs++;
			}
		}
	}

	void GameScene::GetPower(int power) {
		while (power--) {
			if (stats.power < 128) {
				stats.power++;
				switch (stats.power) {
					case 8:
					case 16:
					case 32:
					case 48:
					case 64:
					case 80:
					case 96:
					case 128: {
						Mix_Chunk* sound = game.assets.GetSound("se_powerup.wav");
						StopSound(sound);
						Mix_PlayChannel(-1, sound, 0);
					}
				}
			}
		}
	}

	void GameScene::GetGraze(int graze) {
		stats.graze += graze;
	}

	void GameScene::GetPoints(int points) {
		while (points--) {
			stats.points++;
			if (stats.points >= 800) {
				if (stats.points % 200 == 0) {
					GetLives(1);
				}
			} else {
				switch (stats.points) {
					case 50:
					case 125:
					case 200:
					case 300:
					case 450: GetLives(1);
				}
			}
		}
	}

}
