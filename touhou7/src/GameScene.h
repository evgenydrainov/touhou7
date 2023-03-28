#pragma once

#include "Stage.h"

#define PLAY_AREA_X 32
#define PLAY_AREA_Y 16

namespace th {

	class Game;

	struct Stats {
		int score;
		int lives;
		int bombs;
		int power;
		int graze;
		int points;
	};

	class GameScene {
	public:
		GameScene(Game& game) : game(game), stage(game, *this) {}

		bool Init();
		void Quit();

		void Update(float delta);
		void Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta);

		int stage_index = 0;
		Stats stats{};
		Stage stage;

	private:
		Game& game;

		void ResetStats();

		SDL_Texture* play_area_surface = nullptr;
	};

}
