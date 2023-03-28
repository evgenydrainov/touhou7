#pragma once

#include <SDL.h>

namespace th {

	class Game;

	class TitleScene {
	public:
		TitleScene(Game& game) : game(game) {}

		bool Init();
		void Quit();

		void Update(float delta);
		void Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta);

	private:
		Game& game;
	};

}
