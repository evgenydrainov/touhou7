#if 0

#include "Game.h"

namespace th {

	struct Stage1_Data {

	};

	void Stage1_Init(Game* ctx, SDL_Renderer* renderer, void* mem) {
		Stage1_Data* data = (Stage1_Data*)mem;


	}

	void Stage1_Quit(Game* ctx, void* mem) {
		Stage1_Data* data = (Stage1_Data*)mem;


	}

	void Stage1_Update(Game* ctx, void* mem, bool visible, float delta) {
		if (!visible) return;

		Stage1_Data* data = (Stage1_Data*)mem;


	}

	void Stage1_Draw(Game* ctx, SDL_Renderer* renderer, void* mem, bool visible, float delta) {
		if (!visible) return;

		Stage1_Data* data = (Stage1_Data*)mem;

		SDL_Texture* texture = ctx->assets.GetTexture("MistyLakeTexture.png");
		//SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);

		{
			SDL_Rect dest{0, (int)ctx->game_scene->stage->time / 4 % (2 * PLAY_AREA_W) - 2 * PLAY_AREA_W, PLAY_AREA_W, 2 * PLAY_AREA_W};
			SDL_Rect src{0, 0, 128, 256};
			SDL_RenderCopy(renderer, texture, &src, &dest);
		}

		{
			SDL_Rect dest{0, (int)ctx->game_scene->stage->time / 4 % (2 * PLAY_AREA_W), PLAY_AREA_W, 2 * PLAY_AREA_W};
			SDL_Rect src{0, 0, 128, 256};
			SDL_RenderCopy(renderer, texture, &src, &dest);
		}
	}

}

#endif
