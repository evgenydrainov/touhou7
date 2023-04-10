#pragma once

#include "Stage.h"

#include <optional>

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
		GameScene(Game& game) : game(game) {}

		bool Init();
		void Quit();

		void Update(float delta);
		void Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta);

		void GetScore(int score);
		void GetLives(int lives);
		void GetBombs(int bombs);
		void GetPower(int power);
		void GetGraze(int graze);
		void GetPoints(int points);

		Stats stats{};
		std::optional<Stage> stage;

	private:
		Game& game;

		void ResetStats();

		SDL_Texture* play_area_surface = nullptr;
		bool paused = false;
	};

}
