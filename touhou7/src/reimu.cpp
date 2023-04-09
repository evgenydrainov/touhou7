#include "Game.h"

#include "cpml.h"

namespace th {

	static PlayerBullet& CreateReimuCard(Game* ctx, float x, float y, float dir, float dmg) {
		PlayerBullet& result = ctx->game_scene->stage->CreatePlayerBullet();

		result.x = x;
		result.y = y;
		result.spd = 16.0f;
		result.dir = dir;
		result.radius = 12.0f;
		result.sc.sprite = ctx->assets.GetSprite("ReimuCard");
		result.dmg = dmg;
		result.type = PLAYER_BULLET_REIMU_CARD;

		return result;
	}

	static PlayerBullet& CreateReimuOrbShot(Game* ctx, float x, float y, float dir, float dmg) {
		PlayerBullet& result = ctx->game_scene->stage->CreatePlayerBullet();

		result.x = x;
		result.y = y;
		result.spd = 12.0f;
		result.dir = dir;
		result.radius = 12.0f;
		result.sc.sprite = ctx->assets.GetSprite("ReimuOrbShot");
		result.dmg = dmg;
		result.type = PLAYER_BULLET_REIMU_ORB_SHOT;

		return result;
	}

	void ReimuShotType(Game* ctx, float delta) {
		const unsigned char* key = SDL_GetKeyboardState(nullptr);

		Player& player = ctx->game_scene->stage->player;

		player.reimu.fire_timer += delta;
		while (player.reimu.fire_timer >= 4.0f) {
			if (player.reimu.fire_queue == 0) {
				if (key[SDL_SCANCODE_Z]) {
					player.reimu.fire_queue = 8;
				}
			}

			if (player.reimu.fire_queue > 0) {
				int card_shot_type = 0;
				int orb_shot_type = 0;

				int power = ctx->game_scene->stats.power;

				if (power >= 128) {
					card_shot_type = 3;
				} else if (power >= 32) {
					card_shot_type = 2;
				} else if (power >= 8) {
					card_shot_type = 1;
				}

				if (power >= 128) {
					orb_shot_type = 3;
				} else if (power >= 80) {
					orb_shot_type = 2;
				} else if (power >= 48) {
					orb_shot_type = 1;
				}

				float min_dps = 75.0f;
				float max_dps = 150.0f;
				float dps = cpml::lerp(min_dps, max_dps, (float)power / (float)MAX_POWER);

				{
					float card_fraction = 2.0f / 3.0f;
					float card_dps = dps * card_fraction;
					float shots_per_sec = 15.0f;

					switch (card_shot_type) {
						case 0: {
							int shot_count = 1;
							float card_dmg = card_dps / shots_per_sec / (float)shot_count;

							CreateReimuCard(ctx, player.x, player.y - 10.0f, 90.0f, card_dmg);
							break;
						}
						case 1: {
							int shot_count = 2;
							float card_dmg = card_dps / shots_per_sec / (float)shot_count;

							for (int i = 0; i < shot_count; i++) {
								CreateReimuCard(ctx, player.x - 8.0f + (float)i * 16.0f, player.y - 10.0f, 90.0f, card_dmg);
							}
							break;
						}
						case 2: {
							int shot_count = 3;
							float card_dmg = card_dps / shots_per_sec / (float)shot_count;

							for (int i = 0; i < shot_count; i++) {
								CreateReimuCard(ctx, player.x, player.y - 10.0f, 90.0f - 5.0f + (float)i * 5.0f, card_dmg);
							}
							break;
						}
						case 3: {
							int shot_count = 4;
							float card_dmg = card_dps / shots_per_sec / (float)shot_count;

							for (int i = 0; i < shot_count; i++) {
								CreateReimuCard(ctx, player.x, player.y - 10.0f, 90.0f - 7.5f + (float)i * 5.0f, card_dmg);
							}
							break;
						}
					}
				}

				{
					int frame = 8 - player.reimu.fire_queue;
					float orb_fraction = 1.0f / 3.0f;
					float orb_dps = dps * orb_fraction;

					switch (orb_shot_type) {
						case 0: {
							int shot_count = 2;
							float shots_per_sec = 60.0f / (4.0f * 4.0f);
							float orb_dmg = orb_dps / shots_per_sec / (float)shot_count;

							if (frame % 4 == 0) {
								for (int i = 0; i < shot_count; i++) {
									CreateReimuOrbShot(ctx, player.x, player.y, 90.0f + 70.0f * ((i == 0) ? -1.0f : 1.0f), orb_dmg);
								}
							}
							break;
						}
						case 1: {
							int shot_count = 4;
							float shots_per_sec = 60.0f / (4.0f * 4.0f);
							float orb_dmg = orb_dps / shots_per_sec / (float)shot_count;

							if (frame % 4 == 0) {
								for (int i = 0; i < 2; i++) {
									CreateReimuOrbShot(ctx, player.x, player.y, 90.0f + 50.0f * ((i == 0) ? -1.0f : 1.0f), orb_dmg);
									CreateReimuOrbShot(ctx, player.x, player.y, 90.0f + 70.0f * ((i == 0) ? -1.0f : 1.0f), orb_dmg);
								}
							}
							break;
						}
						case 2: {
							int shot_count = 2;
							float shots_per_sec = 15.0f;
							float orb_dmg = orb_dps / shots_per_sec / (float)shot_count;

							float off = 45.0f + 15.0f * (float)(frame % 3);

							for (int i = 0; i < shot_count; i++) {
								CreateReimuOrbShot(ctx, player.x, player.y, 90.0f + ((i == 0) ? -off : off), orb_dmg);
							}
							break;
						}
						case 3: {
							int shot_count = 2;
							float shots_per_sec = 15.0f;
							float orb_dmg = orb_dps / shots_per_sec / (float)shot_count;

							float off = 30.0f + 15.0f * (float)(frame % 4);

							for (int i = 0; i < shot_count; i++) {
								CreateReimuOrbShot(ctx, player.x, player.y, 90.0f + ((i == 0) ? -off : off), orb_dmg);
							}
							break;
						}
					}
				}

				Mix_Chunk* sound = ctx->assets.GetSound("se_plst00.wav");
				StopSound(sound);
				Mix_PlayChannel(-1, sound, 0);

				player.reimu.fire_queue--;
			}

			player.reimu.fire_timer -= 4.0f;
		}

		//if (key[SDL_SCANCODE_Z] || player.reimu.fire_queue > 0) {
		//	Mix_Chunk* sound = ctx->assets.GetSound("se_plst00.wav");
		//	if (!SoundPlaying(sound)) {
		//		Mix_PlayChannel(-1, sound, 0);
		//	}
		//}
	}

	void ReimuBomb(Game* ctx) {

	}

}
