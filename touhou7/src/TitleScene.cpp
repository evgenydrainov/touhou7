#include "TitleScene.h"

#include "Game.h"

#include "external/stb_sprintf.h"

namespace th {

	bool TitleScene::Init() {
		return true;
	}

	void TitleScene::Quit() {
	
	}

	void TitleScene::Update(float delta) {
		const unsigned char* key = SDL_GetKeyboardState(nullptr);

		if (key[SDL_SCANCODE_RETURN]) {
			game.GoToScene(GAME_SCENE);
		}
		if (key[SDL_SCANCODE_1]) {
			game.skip_to_midboss = 1;
		}
		if (key[SDL_SCANCODE_2]) {
			game.skip_to_boss = 1;
		}

		if (game.key_pressed[SDL_SCANCODE_LEFT]) game.stage_index--;
		if (game.key_pressed[SDL_SCANCODE_RIGHT]) game.stage_index++;
		if (game.key_pressed[SDL_SCANCODE_UP]) game.stage_index -= 100;
		if (game.key_pressed[SDL_SCANCODE_DOWN]) game.stage_index += 100;
	}

	void TitleScene::Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta) {
		SDL_SetRenderTarget(renderer, target);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			DrawTextBitmap(renderer, game.assets.fntMain, "Enter to start", 16, 16);
			DrawTextBitmap(renderer, game.assets.fntMain, game.skip_to_midboss ? "Do skip to midboss" : "Don't skip to midboss", 16, 32);
			DrawTextBitmap(renderer, game.assets.fntMain, game.skip_to_boss ? "Do skip to boss" : "Don't skip to boss", 16, 48);
			{
				char buf[20];
				stbsp_snprintf(buf, 20, "Stage %d", game.stage_index);
				DrawTextBitmap(renderer, game.assets.fntMain, buf, 16, 64);
			}
		}
	}

}
