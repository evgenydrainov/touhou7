#pragma once

#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>

#include <unordered_map>
#include <string>

namespace th {

	class Game;

	struct SpriteData {
		SDL_Texture* texture;
		int u;
		int v;
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

	struct ScriptData {
		std::vector<char> buffer;
		int stage_index;
	};

	void DrawSprite(SDL_Renderer* renderer, SpriteData* sprite, int frame_index, float x, float y, float angle = 0.0f, float xscale = 1.0f, float yscale = 1.0f, SDL_Color color = {255, 255, 255, 255});

	void DrawTextBitmap(SDL_Renderer* renderer, SpriteFont* font, const char* text, int x, int y);

	void StopSound(Mix_Chunk* sound);

	bool SoundPlaying(Mix_Chunk* sound);

	class Assets {
	public:
		bool LoadAssets(SDL_Renderer* renderer);
		void UnloadAssets();

		SpriteData* GetSprite(const std::string& name);
		SDL_Texture* GetTexture(const std::string& name);
		Mix_Chunk* GetSound(const std::string& name);

		const std::unordered_map<std::string, ScriptData*>& GetScripts() const { return scripts; }

		SpriteFont* fntMain = nullptr;
		TTF_Font* fntCirno = nullptr;

	private:
		bool LoadTextureIfNotLoaded(const std::string& fname, SDL_Renderer* renderer);
		bool LoadScriptIfNotLoaded(const std::string& fname, int stage_index);
		bool LoadSoundIfNotLoaded(const std::string& fname);

		std::string assetsFolder = "Assets/";

		std::unordered_map<std::string, SDL_Texture*> textures;
		std::unordered_map<std::string, SpriteData*> sprites; // todo: just use SpriteData, they won't be moved in memory
															  // actual todo: replace this with a proper asset manager
		std::unordered_map<std::string, ScriptData*> scripts;
		std::unordered_map<std::string, Mix_Chunk*> sounds;

		SDL_Texture* default_texture = nullptr;
		SpriteData* default_sprite = nullptr;
	};

}
