#include "TitleScene.h"

#include "Game.h"

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
	}

	void TitleScene::Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta) {
		SDL_SetRenderTarget(renderer, target);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			DrawText(renderer, &game.assets.fntMain, "Enter to start", 16, 16);
			DrawText(renderer, &game.assets.fntMain, game.skip_to_midboss ? "Do skip to midboss" : "Don't skip to midboss", 16, 32);
			DrawText(renderer, &game.assets.fntMain, game.skip_to_boss ? "Do skip to boss" : "Don't skip to boss", 16, 48);
		}
		SDL_SetRenderTarget(renderer, nullptr);
	}

}
