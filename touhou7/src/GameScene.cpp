#include "GameScene.h"

#include "Game.h"

#include "common.h"

namespace th {

	bool GameScene::Init() {
		if (!(play_area_surface = SDL_CreateTexture(game.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, PLAY_AREA_W, PLAY_AREA_H))) {
			TH_SHOW_ERROR("couldn't create play area surface : %s", SDL_GetError());
			return false;
		}

		ResetStats();

		stage.Init();

		return true;
	}

	void GameScene::Quit() {
		stage.Quit();
	}

	void GameScene::Update(float delta) {
		stage.Update(delta);
	}

	void GameScene::ResetStats() {
		stats = {};
		stats.lives = game.options.starting_lives;
		stats.bombs = GetCharacterData(game.player_character)->starting_bombs;
	}

	void GameScene::Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta) {
		stage.Draw(renderer, play_area_surface, delta);

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
				SDL_RenderCopy(renderer, play_area_surface, nullptr, &dest);
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
					snprintf(buf, 10, "%09d", 0);
					DrawText(renderer, &game.assets.fntMain, buf, x, y);
				}
				// score
				{
					char buf[10];
					snprintf(buf, 10, "%09d", stats.score);
					DrawText(renderer, &game.assets.fntMain, buf, x, y + 16);
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

					char buf[4];
					if (stats.power >= MAX_POWER) {
						strncpy_s(buf, 4, "MAX", 3);
					} else {
						snprintf(buf, 4, "%d", stats.power);
					}
					DrawText(renderer, &game.assets.fntMain, buf, rect.x, rect.y);
				}
				// graze
				{
					char buf[10];
					snprintf(buf, 10, "%d", stats.graze);
					DrawText(renderer, &game.assets.fntMain, buf, x, y + 7 * 16);
				}
				// points
				{
					char buf[10];
					snprintf(buf, 10, "%d/%d", stats.points, 0);
					DrawText(renderer, &game.assets.fntMain, buf, x, y + 8 * 16);
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
		}
		SDL_SetRenderTarget(renderer, nullptr);
	}

}
