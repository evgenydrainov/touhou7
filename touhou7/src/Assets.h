#pragma once

#include <SDL.h>

#include <unordered_map>
#include <string>

namespace th
{
	class Game;

	struct SpriteData
	{
		SDL_Texture* texture;
		int x;
		int y;
		int width;
		int height;
		int xorigin;
		int yorigin;
		int frame_count;
		int frames_in_row;
		float anim_spd;
		int loop_frame;
	};

	struct SpriteFont {
		SDL_Texture* texture;
		unsigned char first;
		int width;
		int height;
		int sep_x;
		int sep_y;
		int frames_in_row;
	};

	void DrawSprite(SDL_Renderer* renderer, const SpriteData* sprite, int frame_index, float x, float y, float angle, float xscale = 1.0f, float yscale = 1.0f, unsigned char r = 255, unsigned char g = 255, unsigned char b = 255, unsigned char a = 255);

	void DrawText(SDL_Renderer* renderer, const SpriteFont* font, const char* text, int x, int y);

	class Assets
	{
	public:
		Assets(Game& game) : game(game) {}

		bool LoadAssets();
		void UnloadAssets();

		SpriteData* GetSprite(const std::string& name) const;
		SDL_Texture* GetTexture(const std::string& name) const;

		const std::unordered_map<std::string, std::vector<char>>& GetScripts() const { return scripts; }

		SpriteFont fntMain{};

	private:
		Game& game;

		bool LoadTextureIfNotLoaded(const std::string& fname);
		bool LoadScriptIfNotLoaded(const std::string& fname);

		std::string assetsFolder = "Assets/";

		std::unordered_map<std::string, SDL_Texture*> textures;
		std::unordered_map<std::string, SpriteData*> sprites; // todo: just use SpriteData, they won't be moved in memory
		std::unordered_map<std::string, std::vector<char>> scripts;
	};
}
