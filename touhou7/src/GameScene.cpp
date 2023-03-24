#include "GameScene.h"

#include "Game.h"

#include "common.h"

namespace th
{
	bool GameScene::Init()
	{
		if (!(play_area_surface = SDL_CreateTexture(game.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, PLAY_AREA_W, PLAY_AREA_H))) {
			TH_SHOW_ERROR("couldn't create play area surface : %s", SDL_GetError());
			return false;
		}

		stage.Init();

		return true;
	}

	void GameScene::Quit()
	{
		stage.Quit();
	}

	void GameScene::Update(float delta)
	{
		stage.Update(delta);
	}

	void GameScene::Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta)
	{
		stage.Draw(renderer, play_area_surface, delta);

		SDL_SetRenderTarget(renderer, target);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			{
				SDL_Texture* texture = game.assets.GetTexture("Background.png");
				SDL_RenderCopy(renderer, texture, nullptr, nullptr);
			}

			{
				SDL_Rect dest;
				dest.x = PLAY_AREA_X;
				dest.y = PLAY_AREA_Y;
				dest.w = PLAY_AREA_W;
				dest.h = PLAY_AREA_H;
				SDL_RenderCopy(renderer, play_area_surface, nullptr, &dest);
			}

			{
				SpriteData* sprite = game.assets.GetSprite("Sidebar");
				float x = (float)(PLAY_AREA_X + PLAY_AREA_W + 16);
				float y = (float)(PLAY_AREA_Y + 32);
				DrawSprite(renderer, sprite, 0, x, y, 0.0f);
			}

			{
				SDL_Texture* texture = game.assets.GetTexture("Logo.png");
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
