#include "Assets.h"

#include "Game.h"

#include "common.h"
#include "cpml.h"

#include <SDL_image.h>

#include <fstream>
#include <sstream>

namespace th
{
	static double AngleToSDL(float angle) {
		return -(double)angle;
	}

	void DrawSprite(SDL_Renderer* renderer, const SpriteData* sprite, int frame_index, float x, float y, float angle, float xscale, float yscale, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
	{
		if (!sprite) return;
		if (!sprite->texture) return;

		int u = sprite->x;
		int v = sprite->y;
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
		dest.x = (int)x - xorigin;
		dest.y = (int)y - yorigin;
		dest.w = w;
		dest.h = h;

		int flip = SDL_FLIP_NONE;
		if (xscale < 0.0f) flip |= SDL_FLIP_HORIZONTAL;
		if (yscale < 0.0f) flip |= SDL_FLIP_VERTICAL;

		SDL_SetTextureColorMod(sprite->texture, r, g, b);
		SDL_SetTextureAlphaMod(sprite->texture, a);
		SDL_RenderCopyEx(renderer, sprite->texture, &src, &dest, AngleToSDL(angle), 0, (SDL_RendererFlip)flip);
	}

	void DrawText(SDL_Renderer* renderer, const SpriteFont* font, const char* text, int x, int y) {
		if (!renderer) return;
		if (!font) return;
		if (!text) return;

		int text_x = x;
		int text_y = y;

		for (const char* ch = text; *ch; ch++) {
			switch (*ch) {
				default: {
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
				case ' ':
					text_x += font->width;
					break;
				case '\n':
					text_x = x;
					text_y += font->height;
			}
		}
	}

	bool Assets::LoadAssets()
	{
		bool result = true;

		{
			std::ifstream texturesFile(assetsFolder + "AllTextures.txt");

			for (std::string line; std::getline(texturesFile, line);) {
				if (line.empty()) continue;

				if (line[0] == ';') continue;

				if (!LoadTextureIfNotLoaded(line)) {
					result = false;
					continue;
				}
			}
		}

		{
			std::ifstream spritesFile(assetsFolder + "AllSprites.txt");

			for (std::string line; std::getline(spritesFile, line);) {
				if (line.empty()) continue;

				if (line[0] == ';') continue;

				std::istringstream stream(line);

				std::string name;
				std::string spritesheet;
				int x = 0;
				int y = 0;
				int width = 0;
				int height = 0;
				int xorigin = 0;
				int yorigin = 0;
				int frame_count = 0;
				int frames_in_row = 0;
				float anim_spd = 0.0f;
				int loop_frame = 0;

				stream >> name >> spritesheet >> x >> y >> width >> height >> xorigin >> yorigin >> frame_count >> frames_in_row >> anim_spd >> loop_frame;

				frame_count = (frame_count > 0) ? frame_count : 1;
				frames_in_row = (frames_in_row > 0) ? frames_in_row : frame_count;

				if (!LoadTextureIfNotLoaded(spritesheet)) {
					result = false;
					continue;
				}

				SDL_Texture* texture = GetTexture(spritesheet);

				SpriteData* sprite = new SpriteData{texture, x, y, width, height, xorigin, yorigin, frame_count, frames_in_row, anim_spd, loop_frame};

				auto lookup = sprites.find(name);
				if (lookup == sprites.end()) {
					sprites.emplace(name, sprite);
				}
			}
		}

		{
			std::ifstream scriptsFile(assetsFolder + "AllScripts.txt");

			for (std::string line; std::getline(scriptsFile, line);) {
				if (line.empty()) {
					continue;
				}

				if (line[0] == ';') {
					continue;
				}

				if (!LoadScriptIfNotLoaded(line)) {
					result = false;
					continue;
				}
			}
		}

		{
			fntMain.texture = GetTexture("font.png");
			fntMain.first = 16;
			fntMain.width = 15;
			fntMain.height = 15;
			fntMain.sep_x = 16;
			fntMain.sep_y = 16;
			fntMain.frames_in_row = 16;
		}

		return result;
	}

	void Assets::UnloadAssets()
	{
		for (auto it = sprites.begin(); it != sprites.end(); ++it) {
			delete it->second;
		}
		sprites.clear();

		for (auto it = textures.begin(); it != textures.end(); ++it) {
			SDL_DestroyTexture(it->second);
		}
		textures.clear();
	}

	SpriteData* Assets::GetSprite(const std::string& name) const
	{
		auto lookup = sprites.find(name);
		if (lookup != sprites.end()) {
			return lookup->second;
		}
		return nullptr;
	}

	SDL_Texture* Assets::GetTexture(const std::string& name) const
	{
		auto lookup = textures.find(name);
		if (lookup != textures.end()) {
			return lookup->second;
		}
		return nullptr;
	}

	bool Assets::LoadTextureIfNotLoaded(const std::string& fname)
	{
		auto lookup = textures.find(fname);
		if (lookup == textures.end()) {
			std::string fullPath = assetsFolder + fname;

			SDL_Surface* surf;

			if (!(surf = IMG_Load(fullPath.c_str()))) {
				TH_LOG_ERROR("IMG_Load failed : %s", IMG_GetError());
				return false;
			}

			SDL_Texture* texture;

			if (!(texture = SDL_CreateTextureFromSurface(game.renderer, surf))) {
				SDL_FreeSurface(surf);
				TH_LOG_ERROR("SDL_CreateTextureFromSurface failed : %s", SDL_GetError());
				return false;
			}

			SDL_FreeSurface(surf);

			textures.emplace(fname, texture);
		}

		return true;
	}

	bool Assets::LoadScriptIfNotLoaded(const std::string& fname)
	{
		auto lookup = scripts.find(fname);
		if (lookup == scripts.end()) {
			std::string fullPath = assetsFolder + fname;

			std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
			if (!file) {
				TH_LOG_ERROR("couldn't load script %s", fname.c_str());
				return false;
			}

			std::streamsize size = file.tellg();
			if (size <= 0) {
				TH_LOG_ERROR("couldn't load script %s", fname.c_str());
				return false;
			}

			file.seekg(0, std::ios::beg);

			std::vector<char> buffer(size);
			file.read(buffer.data(), size);

			scripts.emplace(fname, buffer);
		}

		return true;
	}
}
