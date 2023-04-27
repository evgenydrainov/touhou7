#if 0

#include "Game.h"

#include "cpml.h"

#include <SDL_image.h>

#define RES 1

#define BG_SURF_W (384/RES)
#define BG_SURF_H (448/RES)

#define FPS 60.0f

#define CONTROL 0

#define DONT_CALL_SDL 0

namespace th {

	struct Stage1_Data {
		SDL_Texture* bg_surf;
		SDL_Surface* bg_texture;
		float fWorldX;
		float fWorldY;
		float fWorldA;
		float fNear;
		float fFar;
		float fFoVHalf;
		float bg_timer;
	};

	static void Clear_BG_Surf(void* pixels) {
		for (int y = 0; y < BG_SURF_H; y++) {
			for (int x = 0; x < BG_SURF_W; x++) {
				((unsigned int*)pixels)[y * BG_SURF_W + x] = 0xFF000000;
			}
		}
	}

	void Stage1_Init(Game* ctx, SDL_Renderer* renderer, void* mem) {
		sizeof(Stage1_Data);

		Stage1_Data* data = (Stage1_Data*)mem;

		data->bg_surf = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, BG_SURF_W, BG_SURF_H);
		data->bg_texture = IMG_Load("Assets/MistyLakeTexture.png");

		void* pixels;
		int pitch;
		SDL_LockTexture(data->bg_surf, 0, &pixels, &pitch);
		Clear_BG_Surf(pixels);
		SDL_UnlockTexture(data->bg_surf);

		data->fWorldX = -0.5f;
		data->fWorldY = 0.0f;
		data->fWorldA = 3.14159f / 2.0f;
		data->fNear = 0.005f;
		data->fFar = 0.20f;
		data->fFoVHalf = 3.14159f / 4.0f;
	}

	void Stage1_Quit(Game* ctx, void* mem) {
		Stage1_Data* data = (Stage1_Data*)mem;

		SDL_FreeSurface(data->bg_texture);
		SDL_DestroyTexture(data->bg_surf);
	}

	void Stage1_Update(Game* ctx, void* mem, bool visible, float delta) {
		if (!visible) return;

		Stage1_Data* data = (Stage1_Data*)mem;

		bool draw = false;
		data->bg_timer += delta;
		while (data->bg_timer >= (60.0f / FPS)) {
			data->bg_timer -= (60.0f / FPS);
			draw = true;
		}

		if (draw)
		{
			const unsigned char* key = SDL_GetKeyboardState(0);
			float fElapsedTime = (60.0f / FPS) / 60.0f;

#if CONTROL
			// Control rendering parameters dynamically
			if (key[SDL_SCANCODE_Q]) data->fNear += 0.1f * fElapsedTime;
			if (key[SDL_SCANCODE_A]) data->fNear -= 0.1f * fElapsedTime;
			
			if (key[SDL_SCANCODE_W]) data->fFar += 0.1f * fElapsedTime;
			if (key[SDL_SCANCODE_S]) data->fFar -= 0.1f * fElapsedTime;
			
			if (key[SDL_SCANCODE_Z]) data->fFoVHalf += 0.1f * fElapsedTime;
			if (key[SDL_SCANCODE_X]) data->fFoVHalf -= 0.1f * fElapsedTime;
#endif

			float fWorldA = data->fWorldA + (ctx->game_scene->stage->screen_shake_x / 1000.0f);
			float fFar = data->fFar + (ctx->game_scene->stage->screen_shake_y / 6000.0f);

			// Create Frustum corner points
			float fFarX1 = data->fWorldX + cosf(fWorldA - data->fFoVHalf) * fFar;
			float fFarY1 = data->fWorldY + sinf(fWorldA - data->fFoVHalf) * fFar;

			float fNearX1 = data->fWorldX + cosf(fWorldA - data->fFoVHalf) * data->fNear;
			float fNearY1 = data->fWorldY + sinf(fWorldA - data->fFoVHalf) * data->fNear;

			float fFarX2 = data->fWorldX + cosf(fWorldA + data->fFoVHalf) * fFar;
			float fFarY2 = data->fWorldY + sinf(fWorldA + data->fFoVHalf) * fFar;

			float fNearX2 = data->fWorldX + cosf(fWorldA + data->fFoVHalf) * data->fNear;
			float fNearY2 = data->fWorldY + sinf(fWorldA + data->fFoVHalf) * data->fNear;

#if DONT_CALL_SDL
			static void* pixels = new char[4 * BG_SURF_W * BG_SURF_H];
#else
			void* pixels;
			int pitch;
			SDL_LockTexture(data->bg_surf, 0, &pixels, &pitch);
#endif

			Clear_BG_Surf(pixels);

			// Starting with furthest away line and work towards the camera point
			for (int y = 0; y < BG_SURF_H; y++)
			{
				// Take a sample point for depth linearly related to rows down screen
				float fSampleDepth = (float)std::max(y, 1) / ((float)BG_SURF_H);

				// Use sample point in non-linear (1/x) way to enable perspective
				// and grab start and end points for lines across the screen
				float fStartX = (fFarX1 - fNearX1) / (fSampleDepth) + fNearX1;
				float fStartY = (fFarY1 - fNearY1) / (fSampleDepth) + fNearY1;
				float fEndX = (fFarX2 - fNearX2) / (fSampleDepth) + fNearX2;
				float fEndY = (fFarY2 - fNearY2) / (fSampleDepth) + fNearY2;

				// Linearly interpolate lines across the screen
				for (int x = 0; x < BG_SURF_W; x++)
				{
					float fSampleWidth = (float)x / (float)BG_SURF_W;
					float fSampleX = (fEndX - fStartX) * fSampleWidth + fStartX;
					float fSampleY = (fEndY - fStartY) * fSampleWidth + fStartY;

					// Wrap sample coordinates to give "infinite" periodicity on maps
					//fSampleX = fmodf(fSampleX, 1.0f);
					//fSampleY = fmodf(fSampleY, 1.0f);

					// Sample symbol and colour from map sprite, and draw the
					// pixel to the screen
					int sample_x = cpml::wrap((int)(fSampleX * 256.0f), 256);
					int sample_y = cpml::wrap((int)(fSampleY * 256.0f), 256);

					float a = cpml::point_distance(data->fWorldX, data->fWorldY, fSampleX, fSampleY);
					a = std::clamp(a, 0.0f, 1.0f);

					union Color {
						unsigned int value;
						struct {
							unsigned char r;
							unsigned char g;
							unsigned char b;
							unsigned char a;
						};
					};

					Color c = { ((unsigned int*)data->bg_texture->pixels)[sample_y * 256 + sample_x] };
					c.r = (unsigned char)cpml::lerp((float)c.r, 200.0f, a);
					c.g = (unsigned char)cpml::lerp((float)c.g, 179.0f, a);
					c.b = (unsigned char)cpml::lerp((float)c.b, 255.0f, a);

					int draw_y = y;
					((unsigned int*)pixels)[draw_y * BG_SURF_W + x] = c.value;

					// Sample symbol and colour from sky sprite, we can use same
					// coordinates, but we need to draw the "inverted" y-location
					//sym = sprSky->SampleGlyph(fSampleX, fSampleY);
					//col = sprSky->SampleColour(fSampleX, fSampleY);
					//Draw(x, (ScreenHeight() / 2) - y, sym, col);
				}
			}

#if DONT_CALL_SDL == 0
			SDL_UnlockTexture(data->bg_surf);
#endif

#if CONTROL
			// Handle user navigation with arrow keys
			if (key[SDL_SCANCODE_LEFT])
				data->fWorldA -= 1.0f * fElapsedTime;

			if (key[SDL_SCANCODE_RIGHT])
				data->fWorldA += 1.0f * fElapsedTime;

			if (key[SDL_SCANCODE_UP])
			{
				data->fWorldX += cosf(data->fWorldA) * 0.2f * fElapsedTime;
				data->fWorldY += sinf(data->fWorldA) * 0.2f * fElapsedTime;
			}

			if (key[SDL_SCANCODE_DOWN])
			{
				data->fWorldX -= cosf(data->fWorldA) * 0.2f * fElapsedTime;
				data->fWorldY -= sinf(data->fWorldA) * 0.2f * fElapsedTime;
			}
#else
			data->fWorldY = 0.02f * (ctx->game_scene->stage->time / 60.0f);
#endif
		}
	}

	void Stage1_Draw(Game* ctx, SDL_Renderer* renderer, void* mem, bool visible, float delta) {
		if (!visible) return;

		Stage1_Data* data = (Stage1_Data*)mem;

		//auto t = std::chrono::high_resolution_clock::now();

		//auto tt = std::chrono::high_resolution_clock::now();
		//std::chrono::duration<double, std::milli> ms_double = tt - t;
		//std::cout << ms_double.count() << std::endl;

#if DONT_CALL_SDL == 0
		SDL_RenderCopy(renderer, data->bg_surf, 0, 0);
#endif
	}

}

#endif
