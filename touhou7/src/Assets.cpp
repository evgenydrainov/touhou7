#include "Assets.h"

#include "Game.h"

#include "common.h"
#include "cpml.h"

#include <SDL_image.h>

#include <fstream>
#include <sstream>

namespace th {

	static double AngleToSDL(float angle) {
		return -(double)angle;
	}

	void DrawSprite(SDL_Renderer* renderer, SpriteData* sprite, int frame_index, float x, float y, float angle, float xscale, float yscale, SDL_Color color) {
		if (!sprite) return;
		if (!sprite->texture) return;

		int u = sprite->u;
		int v = sprite->v;
		int w = sprite->width;
		int h = sprite->height;
		int xorigin = sprite->xorigin;
		int yorigin = sprite->yorigin;
		int frame_count = sprite->frame_count;
		int frames_in_row = sprite->frames_in_row;

		frame_index = std::clamp(frame_index, 0, frame_count);

		int cell_x = frame_index % frames_in_row;
		int cell_y = frame_index / frames_in_row;

		SDL_Rect src;
		src.x = u + cell_x * w;
		src.y = v + cell_y * h;
		src.w = w;
		src.h = h;

		SDL_Rect dest;
		dest.x = (int) (x - (float)xorigin * fabsf(xscale));
		dest.y = (int) (y - (float)yorigin * fabsf(yscale));
		dest.w = (int) ((float)w * fabsf(xscale));
		dest.h = (int) ((float)h * fabsf(yscale));

		//SDL_FRect dest;
		//dest.x = x - (float)xorigin * fabsf(xscale);
		//dest.y = y - (float)yorigin * fabsf(yscale);
		//dest.w = (float)w * fabsf(xscale);
		//dest.h = (float)h * fabsf(yscale);

		int flip = SDL_FLIP_NONE;
		if (xscale < 0.0f) flip |= SDL_FLIP_HORIZONTAL;
		if (yscale < 0.0f) flip |= SDL_FLIP_VERTICAL;

		SDL_Point center;
		center.x = (int) ((float)xorigin * fabsf(xscale));
		center.y = (int) ((float)yorigin * fabsf(yscale));

		//SDL_FPoint center;
		//center.x = (float)xorigin * fabsf(xscale);
		//center.y = (float)yorigin * fabsf(yscale);

		SDL_SetTextureColorMod(sprite->texture, color.r, color.g, color.b);
		SDL_SetTextureAlphaMod(sprite->texture, color.a);
		//SDL_SetTextureScaleMode(sprite->texture, SDL_ScaleModeLinear);
		SDL_RenderCopyEx(renderer, sprite->texture, &src, &dest, AngleToSDL(angle), &center, (SDL_RendererFlip)flip);
	}

	void DrawTextBitmap(SDL_Renderer* renderer, SpriteFont* font, const char* text, int x, int y) {
		if (!renderer) return;
		if (!font) return;
		if (!text) return;

		int text_x = x;
		int text_y = y;

		for (const char* ch = text; *ch; ch++) {
			if (*ch == '\n') {
				text_x = x;
				text_y += font->height;
				continue;
			}

			if (*ch != ' ') {
				int frame_index = *ch - font->first;
				SDL_Rect src;
				src.x = (frame_index % font->frames_in_row) * font->sep_x;
				src.y = (frame_index / font->frames_in_row) * font->sep_y;
				src.w = font->width;
				src.h = font->height;
				SDL_Rect dest;
				dest.x = text_x;
				dest.y = text_y;
				dest.w = font->width;
				dest.h = font->height;
				SDL_RenderCopy(renderer, font->texture, &src, &dest);
			}

			text_x += font->width;
		}
	}

	void StopSound(Mix_Chunk* sound) {
		for (int i = 0; i < MIX_CHANNELS; i++) {
			if (Mix_Playing(i)) {
				if (Mix_GetChunk(i) == sound) {
					Mix_HaltChannel(i);
				}
			}
		}
	}

	bool SoundPlaying(Mix_Chunk* sound) {
		for (int i = 0; i < MIX_CHANNELS; i++) {
			if (Mix_Playing(i)) {
				if (Mix_GetChunk(i) == sound) {
					return true;
				}
			}
		}
		return false;
	}

	template <typename F>
	static bool ReadTextFile(const std::string& assetsFolder, const std::string& fname, const F& f) {
		std::ifstream file(assetsFolder + fname);

		if (!file) {
			TH_LOG_ERROR("couldn't open %s", fname.c_str());
			return false;
		}

		for (std::string line; std::getline(file, line);) {
			if (line.empty()) continue;

			if (line[0] == ';') continue;

			f(line);
		}

		return true;
	}

	bool Assets::LoadAssets(SDL_Renderer* renderer) {
		bool result = true;

		// default texture & sprite
		{
			SDL_Surface* surface = SDL_CreateRGBSurface(0, 16, 16, 24, 0, 0, 0, 0);
			SDL_FillRect(surface, nullptr, 0xFFFFFFFF);
			default_texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);
			textures["<default>"] = default_texture;

			default_sprite = new SpriteData{default_texture, 0, 0, 16, 16, 0, 0, 1, 1, 0.0f, 0};
			sprites["<default>"] = default_sprite;
		}

		// textures

		ReadTextFile(assetsFolder, "AllTextures.txt", [this, renderer, &result](const std::string& line) {
			if (!LoadTextureIfNotLoaded(line, renderer)) {
				result = false;
			}
		});

		// sprites

		ReadTextFile(assetsFolder, "AllSprites.txt", [this, renderer, &result](const std::string& line) {
			std::istringstream stream(line);

			std::string name;
			std::string spritesheet;
			int u = 0;
			int v = 0;
			int width = 0;
			int height = 0;
			int xorigin = 0;
			int yorigin = 0;
			int frame_count = 0;
			int frames_in_row = 0;
			float anim_spd = 0.0f;
			int loop_frame = 0;

			stream >> name >> spritesheet >> u >> v >> width >> height >> xorigin >> yorigin >> frame_count >> frames_in_row >> anim_spd >> loop_frame;

			frame_count = (frame_count > 0) ? frame_count : 1;
			frames_in_row = (frames_in_row > 0) ? frames_in_row : frame_count;

			if (!LoadTextureIfNotLoaded(spritesheet, renderer)) {
				result = false;
				return;
			}

			SDL_Texture* texture = GetTexture(spritesheet);

			auto lookup = sprites.find(name);
			if (lookup == sprites.end()) {
				SpriteData* sprite = new SpriteData{texture, u, v, width, height, xorigin, yorigin, frame_count, frames_in_row, anim_spd, loop_frame};

				sprites.emplace(name, sprite);
			}
		});

		// lua scripts

		ReadTextFile(assetsFolder, "AllScripts.txt", [this, &result](const std::string& line) {
			std::istringstream stream(line);

			std::string fname;
			int stage_index = -1;

			stream >> fname >> stage_index;

			if (!LoadScriptIfNotLoaded(fname, stage_index)) {
				result = false;
				return;
			}
		});

		// gui font (hard coded)
		{
			fntMain = new SpriteFont{GetTexture("font.png"), 16, 15, 15, 16, 16, 16};
		}

		{
			std::string fullPath = assetsFolder + "Cirno.ttf";

			fntCirno = TTF_OpenFont(fullPath.c_str(), 18);
		}

		// sounds

		ReadTextFile(assetsFolder, "AllSounds.txt", [this, &result](const std::string& line) {
			std::istringstream stream(line);

			std::string fname;
			float volume = 1.0f;

			stream >> fname >> volume;

			if (!LoadSoundIfNotLoaded(fname)) {
				result = false;
				return;
			}

			Mix_Chunk* sound = GetSound(fname);
			Mix_VolumeChunk(sound, (int)(volume * (float)MIX_MAX_VOLUME));
		});

		return result;
	}

	void Assets::UnloadAssets() {
		for (auto it = sounds.begin(); it != sounds.end(); ++it) {
			Mix_FreeChunk(it->second);
		}
		sounds.clear();

		TTF_CloseFont(fntCirno);
		delete fntMain;

		for (auto it = scripts.begin(); it != scripts.end(); ++it) {
			delete it->second;
		}
		scripts.clear();

		for (auto it = sprites.begin(); it != sprites.end(); ++it) {
			delete it->second;
		}
		sprites.clear();

		for (auto it = textures.begin(); it != textures.end(); ++it) {
			SDL_DestroyTexture(it->second);
		}
		textures.clear();
	}

	SpriteData* Assets::GetSprite(const std::string& name) {
		auto lookup = sprites.find(name);
		if (lookup != sprites.end()) {
			return lookup->second;
		}
		return default_sprite;
	}

	SDL_Texture* Assets::GetTexture(const std::string& name) {
		auto lookup = textures.find(name);
		if (lookup != textures.end()) {
			return lookup->second;
		}
		return default_texture;
	}

	Mix_Chunk* Assets::GetSound(const std::string& name) {
		auto lookup = sounds.find(name);
		if (lookup != sounds.end()) {
			return lookup->second;
		}
		return nullptr;
	}

	bool Assets::LoadTextureIfNotLoaded(const std::string& fname, SDL_Renderer* renderer) {
		auto lookup = textures.find(fname);
		if (lookup == textures.end()) {
			std::string fullPath = assetsFolder + fname;

			SDL_Texture* texture;

			if (!(texture = IMG_LoadTexture(renderer, fullPath.c_str()))) {
				TH_LOG_ERROR("couldn't load texture %s: %s", fname.c_str(), IMG_GetError());
				return false;
			}

			textures.emplace(fname, texture);
		}

		return true;
	}

	bool Assets::LoadScriptIfNotLoaded(const std::string& fname, int stage_index) {
		auto lookup = scripts.find(fname);
		if (lookup == scripts.end()) {
			std::string fullPath = assetsFolder + fname;

			std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
			if (!file) {
				TH_LOG_ERROR("couldn't open script file %s", fname.c_str());
				return false;
			}

			std::streamsize size = file.tellg();
			if (size <= 0) {
				TH_LOG_ERROR("script file %s size is %d", fname.c_str(), (int)size);
				return false;
			}

			file.seekg(0, std::ios::beg);

			ScriptData* script = new ScriptData{};
			script->buffer.resize((size_t)size);
			file.read(script->buffer.data(), size);
			script->stage_index = stage_index;

			scripts.emplace(fname, script);
		}

		return true;
	}

	bool Assets::LoadSoundIfNotLoaded(const std::string& fname) {
		auto lookup = sounds.find(fname);
		if (lookup == sounds.end()) {
			std::string fullPath = assetsFolder + fname;

			Mix_Chunk* chunk;

			if (!(chunk = Mix_LoadWAV(fullPath.c_str()))) {
				TH_LOG_ERROR("couldn't load sound %s: %s", fname.c_str(), Mix_GetError());
				return false;
			}

			sounds.emplace(fname, chunk);
		}

		return true;
	}

}
