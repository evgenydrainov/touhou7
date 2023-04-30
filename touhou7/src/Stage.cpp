#include "Stage.h"

#include "Game.h"

#include "common.h"
#include "cpml.h"
#include "external/stb_sprintf.h"

namespace th {

	int CreateCoroutine(lua_State* L, lua_State* main_L);

	bool Stage::Init() {
		CreatePlayer();

		InitLua();

		{
			stage_memory = new unsigned char[1000]{};

			StageData* data = GetStageData(game.stage_index);

			if (data->init) {
				(*data->init)(&game, game.renderer, stage_memory);
			}
		}

		return true;
	}

	void Stage::Quit() {
		game.skip_to_midboss = false;
		game.skip_to_boss = false;

		{
			StageData* data = GetStageData(game.stage_index);
			if (data->quit) {
				(*data->quit)(&game, stage_memory);
			}
		}

		delete[] stage_memory;

		for (Enemy& enemy : enemies) {
			FreeEnemy(enemy);
		}
		enemies.clear();

		for (Bullet& bullet : bullets) {
			FreeBullet(bullet);
		}
		bullets.clear();

		FreeBoss();

		lua_close(L); // crashes if L is null
	}

	bool CallLuaFunction(lua_State* L, int ref, instance_id id);
	void UpdateCoroutine(lua_State* L, int* coroutine, instance_id id);

#define PLAYER_DEATH_TIME      15.0f
#define PLAYER_APPEAR_TIME     30.0f
#define PLAYER_RESPAWN_IFRAMES 120.0f
#define PLAYER_BOMB_TIME       (2.5f * 60.0f)

	template <typename Object>
	static Object* FindClosest(std::vector<Object>& storage, float x, float y) {
		float closest_dist = 1'000'000.0f;
		Object* result = nullptr;
		for (Object& object : storage) {
			float dist = cpml::point_distance(x, y, object.x, object.y);
			if (dist < closest_dist) {
				result = &object;
				closest_dist = dist;
			}
		}
		return result;
	}

	void Stage::UpdatePlayer(float delta) {
		const unsigned char* key = SDL_GetKeyboardState(nullptr);

		player.hsp = 0.0f;
		player.vsp = 0.0f;
		player.focus = false;

		CharacterData* character = GetCharacterData(game.player_character);

		switch (player.state) {
			case PlayerState::Normal: {
				player.focus = key[SDL_SCANCODE_LSHIFT];

				float xmove = 0.0f;
				float ymove = 0.0f;

				if (key[SDL_SCANCODE_LEFT]) xmove -= 1.0f;
				if (key[SDL_SCANCODE_RIGHT]) xmove += 1.0f;
				if (key[SDL_SCANCODE_UP]) ymove -= 1.0f;
				if (key[SDL_SCANCODE_DOWN]) ymove += 1.0f;

				float len = cpml::point_distance(0.0f, 0.0f, xmove, ymove);
				if (len != 0.0f) {
					xmove /= len;
					ymove /= len;
				}

				float spd = player.focus ? character->focus_spd : character->move_spd;
				player.hsp = xmove * spd;
				player.vsp = ymove * spd;

				if (character->shot_type) {
					(*character->shot_type)(&game, delta);
				}

				if (key[SDL_SCANCODE_X]) {
					if (player.bomb_timer == 0.0f) {
						if (scene.stats.bombs > 0) {
							if (character->bomb) {
								(*character->bomb)(&game);
							}
							scene.stats.bombs--;
							player.bomb_timer = PLAYER_BOMB_TIME;
						}
					}
				}

				player.iframes = std::max(player.iframes - delta, 0.0f);

				SpriteComponent& sc = player.sc;

				if (xmove != 0.0f) {
					if (sc.sprite != character->turn_spr) {
						sc.sprite = character->turn_spr;
						sc.frame_index = 0.0f;
					}
					if (xmove > 0.0f) {
						player.facing = 1.0f;
					} else {
						player.facing = -1.0f;
					}
				}

				if (xmove == 0.0f && sc.sprite == character->turn_spr) {
					if (sc.frame_index > (float)sc.sprite->loop_frame - 1.0f) {
						sc.frame_index = (float)sc.sprite->loop_frame - 1.0f;
					}
					sc.frame_index -= sc.sprite->anim_spd * delta;
					if (sc.frame_index < 0.0f) {
						sc.sprite = character->idle_spr;
						sc.frame_index = 0.0f;
					}
				} else {
					sc.frame_index += sc.sprite->anim_spd * delta;
					if ((int)sc.frame_index >= sc.sprite->frame_count) {
						SpriteData* sprite = sc.sprite;
						sc.frame_index = (float)sprite->loop_frame + fmodf(sc.frame_index - (float)sprite->loop_frame, (float)(sprite->frame_count - sprite->loop_frame));
					}
				}
				break;
			}
			case PlayerState::Dying: {
				if (key[SDL_SCANCODE_X]) {
					if ((PLAYER_DEATH_TIME - player.timer) < character->deathbomb_time) {
						if (player.bomb_timer == 0.0f) {
							if (scene.stats.bombs > 0) {
								if (character->bomb) {
									(*character->bomb)(&game);
								}
								scene.stats.bombs--;
								player.bomb_timer = PLAYER_BOMB_TIME;
								player.state = PlayerState::Normal;
								player.iframes = PLAYER_RESPAWN_IFRAMES;
							}
						}
					}
				}

				player.timer = std::max(player.timer - delta, 0.0f);
				if (player.timer == 0.0f) {
					if (scene.stats.lives > 0) {
						scene.stats.lives--;

						int drop = std::min(scene.stats.power, 16);
						scene.stats.power -= drop;
						drop = std::min(drop, 12);
						while (drop > 0) {
							int type;
							if (drop >= 8) {
								drop -= 8;
								type = PICKUP_BIGP;
							} else {
								drop--;
								type = PICKUP_POWER;
							}
							float x = player.x + random.range(-50.0f, 50.0f);
							float y = player.y + random.range(-50.0f, 50.0f);
							CreatePickup(x, y, type);
						}
					} else {
						//g_screen->GameOver();
						CreatePickup(player.x, player.y, PICKUP_FULL_POWER);
					}
					CreatePlayer(true);
					return;
				}
				break;
			}
			case PlayerState::Appearing: {
				player.timer = std::max(player.timer - delta, 0.0f);
				if (player.timer == 0.0f) {
					player.state = PlayerState::Normal;
				}
				break;
			}
		}

		player.bomb_timer = std::max(player.bomb_timer - delta, 0.0f);

		if (player.focus) {
			player.hitbox_alpha = cpml::approach(player.hitbox_alpha, 1.0f, 0.1f * delta);
		} else {
			player.hitbox_alpha = cpml::approach(player.hitbox_alpha, 0.0f, 0.1f * delta);
		}
	}

	void Stage::CallCoroutines() {
		UpdateCoroutine(L, &coroutine, -1);

		if (boss_exists) {
			UpdateCoroutine(L, &boss.coroutine, boss.id);
		}

		for (size_t i = 0, n = enemies.size(); i < n; i++) {
			UpdateCoroutine(L, &enemies[i].coroutine, enemies[i].id);
		}

		for (size_t i = 0, n = bullets.size(); i < n; i++) {
			UpdateCoroutine(L, &bullets[i].coroutine, bullets[i].id);
		}
	}

	void Stage::Update(float delta) {
		// update
		{
			UpdatePlayer(delta);

			if (boss_exists) {
				UpdateBoss(delta);
			}

			for (Pickup& pickup : pickups) {
				if (pickup.homing) {
					float spd = 8.0f;
					float dir = cpml::point_direction(pickup.x, pickup.y, player.x, player.y);
					pickup.hsp = cpml::lengthdir_x(spd, dir);
					pickup.vsp = cpml::lengthdir_y(spd, dir);

					if (player.state == PlayerState::Dying) {
						pickup.hsp = 0.0f;
						pickup.vsp = -1.5f;
						pickup.homing = false;
					}
				} else {
					pickup.hsp = 0.0f;
					pickup.vsp += 0.025f * delta;
					pickup.vsp = std::min(pickup.vsp, 2.0f);
				}
			}

			for (Bullet& bullet : bullets) {
				switch (bullet.type) {
					case ProjectileType::Lazer: {
						if (bullet.lazer_timer < bullet.lazer_time) {
							bullet.lazer_timer += delta;
							bullet.length = cpml::lerp(0.0f, bullet.target_length, bullet.lazer_timer / bullet.lazer_time);
						} else {
							bullet.length = bullet.target_length;
						}
						break;
					}
					case ProjectileType::SLazer: {
						if (bullet.lazer_timer < bullet.lazer_time) {
							bullet.lazer_timer += delta;
							bullet.length = 0.0f;
						} else {
							bullet.length = bullet.target_length;
						}
						break;
					}
				}
			}

			for (PlayerBullet& bullet : player_bullets) {
				switch (bullet.type) {
					case PLAYER_BULLET_REIMU_CARD: {
						bullet.rotation += 16.0f * delta;
						break;
					}
					case PLAYER_BULLET_REIMU_ORB_SHOT: {
						float target_x = 0.0f;
						float target_y = 0.0f;
						float target_dist = 1'000'000.0f;
						bool home = false;

						Enemy* enemy = FindClosest(enemies, bullet.x, bullet.y);
						if (enemy) {
							target_x = enemy->x;
							target_y = enemy->y;
							target_dist = cpml::point_distance(bullet.x, bullet.y, enemy->x, enemy->y);
							home = true;
						}

						if (boss_exists) {
							float boss_dist = cpml::point_distance(bullet.x, bullet.y, boss.x, boss.y);
							if (boss_dist < target_dist) {
								target_x = boss.x;
								target_y = boss.y;
								target_dist = boss_dist;
								home = true;
							}
						}

						if (home) {
							// @goofy
							float hsp = cpml::lengthdir_x(bullet.spd, bullet.dir);
							float vsp = cpml::lengthdir_y(bullet.spd, bullet.dir);
							float dx = target_x - bullet.x;
							float dy = target_y - bullet.y;
							dx = std::clamp(dx, -12.0f, 12.0f);
							dy = std::clamp(dy, -12.0f, 12.0f);
							hsp = cpml::approach(hsp, dx, 1.5f * delta);
							vsp = cpml::approach(vsp, dy, 1.5f * delta);
							bullet.spd = cpml::point_distance(0.0f, 0.0f, hsp, vsp);
							bullet.dir = cpml::point_direction(0.0f, 0.0f, hsp, vsp);

							//float target_dir = cpml::point_direction(bullet.x, bullet.y, target_x, target_y);
							//bullet.dir += cpml::angle_difference(target_dir, bullet.dir) / 20.0f;
						} else {
							if (bullet.spd < 10.0f) {
								bullet.spd += 1.0f * delta;
							}
						}
						break;
					}
				}
			}
		}

		// physics
		{
			float physics_update_rate = 1.0f / 300.0f; // 300 fps
			float physics_timer = delta * /*g_stage->gameplay_delta*/1.0f;
			while (physics_timer > 0.0f) {
				float pdelta = std::min(physics_timer, physics_update_rate * 60.0f);
				PhysicsUpdate(pdelta);
				physics_timer -= pdelta;
			}
		}

		// scripts
		{
			coro_update_timer += delta;
			while (coro_update_timer >= 1.0f) {
				CallCoroutines();
				coro_update_timer -= 1.0f;
			}

			for (size_t i = 0, n = enemies.size(); i < n; i++) {
				CallLuaFunction(L, enemies[i].update_callback, enemies[i].id);
			}
		}

		// cleanup
		{
			for (auto enemy = enemies.begin(); enemy != enemies.end();) {
				if (enemy->dead) {
					FreeEnemy(*enemy);
					enemy = enemies.erase(enemy);
					continue;
				}
				++enemy;
			}

			for (auto bullet = bullets.begin(); bullet != bullets.end();) {
				if (bullet->dead) {
					FreeBullet(*bullet);
					bullet = bullets.erase(bullet);
					continue;
				}
				++bullet;
			}

			if (player.dead) {
				CreatePlayer(true);
			}

			if (boss_exists) {
				if (boss.dead) {
					FreeBoss();
					boss_exists = false;
				}
			}
		}

		// late update
		{
			player.x = std::clamp(player.x, 0.0f, (float)PLAY_AREA_W - 1.0f);
			player.y = std::clamp(player.y, 0.0f, (float)PLAY_AREA_H - 1.0f);

			for (auto bullet = bullets.begin(); bullet != bullets.end();) {
				float x = bullet->x;
				float y = bullet->y;
				if (x < 0.0f || x >= (float)PLAY_AREA_W || y < 0.0f || y >= (float)PLAY_AREA_H) {
					FreeBullet(*bullet);
					bullet = bullets.erase(bullet);
					continue;
				}
				bullet->lifetime += delta;
				if (bullet->lifetime > 60.0f * 60.0f) {
					FreeBullet(*bullet);
					bullet = bullets.erase(bullet);
					continue;
				}
				if (bullet->type == ProjectileType::SLazer) {
					if (bullet->lifetime >= bullet->lazer_lifetime) {
						FreeBullet(*bullet);
						bullet = bullets.erase(bullet);
						continue;
					}
				}
				++bullet;
			}

			for (auto pickup = pickups.begin(); pickup != pickups.end();) {
				float x = pickup->x;
				float y = pickup->y;
				if (x < 0.0f || x >= (float)PLAY_AREA_W || y < -50.0f || y >= (float)PLAY_AREA_H) {
					pickup = pickups.erase(pickup);
					continue;
				}
				++pickup;
			}

			for (auto b = player_bullets.begin(); b != player_bullets.end();) {
				float x = b->x;
				float y = b->y;
				if (x < 0.0f || x >= (float)PLAY_AREA_W || y < 0.0f || y >= (float)PLAY_AREA_H) {
					b = player_bullets.erase(b);
					continue;
				}
				++b;
			}

			for (auto e = enemies.begin(); e != enemies.end();) {
				float x = e->x;
				float y = e->y;
				if (x < 0.0f || x >= (float)PLAY_AREA_W || y < 0.0f || y >= (float)PLAY_AREA_H) {
					FreeEnemy(*e);
					e = enemies.erase(e);
					continue;
				}
				++e;
			}
		}

		// animate
		{
			if (boss_exists) {
				UpdateSpriteComponent(boss.sc, delta);
			}

			for (Enemy& enemy : enemies) {
				UpdateSpriteComponent(enemy.sc, delta);
			}
		}

		{
			bool spellcard_bg_on_screen = false;
			if (boss_exists) {
				BossData* data = GetBossData(boss.type_index);
				PhaseData* phase = GetPhaseData(data, boss.phase_index);

				if (phase->type == PHASE_SPELLCARD) {
					if (boss.state != BossState::WaitingEnd) {
						spellcard_bg_on_screen = true;
					}
				}
			}

			if (spellcard_bg_on_screen) {
				spellcard_bg_alpha = cpml::approach(spellcard_bg_alpha, 1.0f, 1.0f / 30.0f * delta);
			} else {
				spellcard_bg_alpha = 0.0f;
			}
		}

		screen_shake_timer = std::max(screen_shake_timer - delta, 0.0f);

		if (screen_shake_timer > 0.0f) {
			float power = screen_shake_power * (screen_shake_timer / screen_shake_time);
			screen_shake_x = game.random.range(-power / 2.0f, power / 2.0f);
			screen_shake_y = game.random.range(-power / 2.0f, power / 2.0f);
		} else {
			screen_shake_x = 0.0f;
			screen_shake_y = 0.0f;
		}

		{
			StageData* stage = GetStageData(game.stage_index);
			if (stage->update) {
				(*stage->update)(&game, stage_memory, spellcard_bg_alpha < 1.0f, delta);
			}
		}

		time += delta;
	}

	void Stage::UpdateSpriteComponent(SpriteComponent& sc, float delta) {
		if (SpriteData* sprite = sc.sprite) {
			int frame_count = sprite->frame_count;
			int loop_frame = sprite->loop_frame;
			float anim_spd = sprite->anim_spd;

			sc.frame_index += anim_spd * delta;
			if ((int)sc.frame_index >= frame_count) {
				sc.frame_index = (float)loop_frame + fmodf(sc.frame_index - (float)loop_frame, (float)(frame_count - loop_frame));
			}
		}
	}

	template <typename T>
	static void MoveObject(T& object, float delta) {
		object.x += cpml::lengthdir_x(object.spd, object.dir) * delta;
		object.y += cpml::lengthdir_y(object.spd, object.dir) * delta;
		object.spd += object.acc * delta;

		if (object.spd < 0.0f) {
			object.spd = 0.0f;
		}
	}

	static bool PlayerVsBullet(Player& player, float player_radius, Bullet& bullet) {
		switch (bullet.type) {
			case ProjectileType::Bullet: {
				return cpml::circle_vs_circle(player.x, player.y, player_radius, bullet.x, bullet.y, bullet.radius);
			}
			case ProjectileType::Lazer:
			case ProjectileType::SLazer: {
				float rect_center_x = bullet.x + cpml::lengthdir_x(bullet.length / 2.0f, bullet.dir);
				float rect_center_y = bullet.y + cpml::lengthdir_y(bullet.length / 2.0f, bullet.dir);
				return cpml::circle_vs_rotated_rect(player.x, player.y, player_radius, rect_center_x, rect_center_y, bullet.thickness, bullet.length, bullet.dir);
			}
		}
		return false;
	}

	void Stage::PhysicsUpdate(float delta) {
		{
			player.x += player.hsp * delta;
			player.y += player.vsp * delta;

			if (boss_exists) {
				MoveObject(boss, delta);
			}

			for (Enemy& enemy : enemies) {
				MoveObject(enemy, delta);
			}

			for (Bullet& bullet : bullets) {
				switch (bullet.type) {
					case ProjectileType::Lazer:
					case ProjectileType::SLazer: {
						if (bullet.lazer_timer >= bullet.lazer_time) {
							MoveObject(bullet, delta);
						}
						break;
					}
					default: {
						MoveObject(bullet, delta);
						break;
					}
				}
			}

			for (Pickup& pickup : pickups) {
				pickup.x += pickup.hsp * delta;
				pickup.y += pickup.vsp * delta;
			}

			for (PlayerBullet& b : player_bullets) {
				MoveObject(b, delta);
			}
		}

		{
			CharacterData* character = GetCharacterData(game.player_character);

			// player vs bullet
			for (auto bullet = bullets.begin(); bullet != bullets.end();) {
				if (PlayerVsBullet(player, character->graze_radius, *bullet)) {
					if (player.state == PlayerState::Normal) {
						if (!bullet->grazed) {
							scene.GetGraze(1);
							PlaySound("se_graze.wav");
							bullet->grazed = true;
						}
					}
				}

				if (PlayerVsBullet(player, player.radius, *bullet)) {
					if (player.state == PlayerState::Normal) {
						if (player.iframes == 0.0f) {
							player.state = PlayerState::Dying;
							player.timer = PLAYER_DEATH_TIME;
							PlaySound("se_pichuun.wav");
							bullets.erase(bullet);
							break;
						}
					}
				}

				++bullet;
			}

			// player vs pickup
			for (auto pickup = pickups.begin(); pickup != pickups.end();) {
				if (cpml::circle_vs_circle(player.x, player.y, character->graze_radius, pickup->x, pickup->y, pickup->radius)) {
					if (player.state == PlayerState::Normal) {
						switch (pickup->type) {
							case PICKUP_POWER:
								scene.GetPower(1);
								scene.GetScore(10);
								break;
							case PICKUP_POINT:      scene.GetPoints(1);        break;
							case PICKUP_BIGP:       scene.GetPower(8);         break;
							case PICKUP_BOMB:       scene.GetBombs(1);         break;
							case PICKUP_FULL_POWER: scene.GetPower(MAX_POWER); break;
							case PICKUP_1UP:        scene.GetLives(1);         break;
							case PICKUP_SCORE:      scene.GetScore(10);        break;
						}
						PlaySound("se_item.wav");
						pickup = pickups.erase(pickup);
						continue;
					}
				}
				++pickup;
			}

			// boss vs bullet
			if (boss_exists) {
				for (auto bullet = player_bullets.begin(); bullet != player_bullets.end();) {
					if (cpml::circle_vs_circle(boss.x, boss.y, boss.radius, bullet->x, bullet->y, bullet->radius)) {
						PlaySound("se_enemy_hit.wav");
						if (boss.state == BossState::Normal) {
							boss.hp -= bullet->dmg;
							if (boss.hp <= 0.0f) {
								EndBossPhase();
								player_bullets.erase(bullet);
								break;
							}
						}
						bullet = player_bullets.erase(bullet);
						continue;
					}
					++bullet;
				}
			}

			// enemy vs bullet
			// @goofy
			for (size_t i = 0, n = enemies.size(); i < n; i++) {
				bool enemy_dead = false;
				for (auto bullet = player_bullets.begin(); bullet != player_bullets.end();) {
					if (cpml::circle_vs_circle(enemies[i].x, enemies[i].y, enemies[i].radius, bullet->x, bullet->y, bullet->radius)) {
						enemies[i].hp -= bullet->dmg;
						bullet = player_bullets.erase(bullet);
						PlaySound("se_enemy_hit.wav");
						if (enemies[i].hp <= 0.0f) {
							PlaySound("se_enemy_die.wav");
							enemy_dead = true;
							break;
						}
						continue;
					}
					++bullet;
				}
				if (enemy_dead) {
					CallLuaFunction(L, enemies[i].death_callback, enemies[i].id);

					FreeEnemy(enemies[i]);
					enemies.erase(enemies.begin() + i);
					i--;
					n--;
				}
			}
		}
	}

	#define BOSS_PHASE_START_TIME 60.0f
	#define BOSS_SPELL_END_TIME   60.0f

	template <typename Object>
	static void LaunchTowardsPoint(Object& object, float target_x, float target_y, float acc) {
		acc = fabsf(acc);
		float dist = cpml::point_distance(object.x, object.y, target_x, target_y);
		object.spd = sqrtf(dist * acc * 2.0f);
		object.acc = -acc;
		object.dir = cpml::point_direction(object.x, object.y, target_x, target_y);
	}

	void Stage::StartBossPhase() {
		BossData* data = GetBossData(boss.type_index);
		PhaseData* phase = GetPhaseData(data, boss.phase_index);

		boss.hp = phase->hp;
		boss.timer = phase->time;
		boss.wait_timer = BOSS_PHASE_START_TIME;
		boss.state = BossState::WaitingStart;

		if (boss.phase_index > 0) {
			LaunchTowardsPoint(boss, BOSS_STARTING_X, BOSS_STARTING_Y, 0.05f);
		}

		if (phase->type == PHASE_SPELLCARD) {
			PlaySound("se_spellcard.wav");
		}
	}

	bool Stage::EndBossPhase() {
		for (Bullet& bullet : bullets) {
			CreatePickup(bullet.x, bullet.y, PICKUP_SCORE);
			FreeBullet(bullet);
		}
		bullets.clear();

		for (Pickup& pickup : pickups) {
			pickup.homing = true;
		}

		if (boss.coroutine != LUA_REFNIL) {
			luaL_unref(L, LUA_REGISTRYINDEX, boss.coroutine);
			boss.coroutine = LUA_REFNIL;
		}

		BossData* data = GetBossData(boss.type_index);
		PhaseData* phase = GetPhaseData(data, boss.phase_index);

		if (phase->type == PHASE_SPELLCARD) {
			// drop some pickups
			for (int i = 0; i < 5; i++) {
				float x = boss.x + random.range(-50.0f, 50.0f);
				float y = boss.y + random.range(-50.0f, 50.0f);
				unsigned char type = (i == 4) ? PICKUP_BIGP : PICKUP_POWER;
				CreatePickup(x, y, type);
			}
		}

		if (boss.phase_index + 1 < data->phase_count) {
			if (phase->type == PHASE_SPELLCARD) {
				boss.state = BossState::WaitingEnd;
				boss.wait_timer = BOSS_SPELL_END_TIME;
			} else {
				boss.phase_index++;
				StartBossPhase();
			}
			PlaySound("se_enemy_die.wav");
		} else {
			for (Pickup& pickup : pickups) {
				pickup.homing = true;
			}

			if (data->type == BOSS_BOSS) {
				PlaySound("se_boss_die.wav");
				ScreenShake(6.0f, 120.0f);
			} else {
				PlaySound("se_enemy_die.wav");
			}

			FreeBoss();
			boss_exists = false;
		}

		return true;
	}

	void Stage::UpdateBoss(float delta) {
		switch (boss.state) {
			case BossState::Normal: {
				boss.timer = std::max(boss.timer - delta, 0.0f);
				if (boss.timer == 0.0f) {
					EndBossPhase();

					if (!boss_exists) {
						return;
					}
				}
				break;
			}
			case BossState::WaitingStart: {
				boss.wait_timer = std::max(boss.wait_timer - delta, 0.0f);
				if (boss.wait_timer == 0.0f) {
					BossData* data = GetBossData(boss.type_index);
					PhaseData* phase = GetPhaseData(data, boss.phase_index);

					boss.state = BossState::Normal;
					lua_getglobal(L, phase->script);
					boss.coroutine = CreateCoroutine(L, L);
				}
				break;
			}
			case BossState::WaitingEnd: {
				boss.wait_timer = std::max(boss.wait_timer - delta, 0.0f);
				if (boss.wait_timer == 0.0f) {
					boss.phase_index++;
					StartBossPhase();
				}
				break;
			}
		}

		if (boss.spd > 0.1f) {
			if (90.0f <= boss.dir && boss.dir < 270.0f) {
				boss.facing = 1.0f;
			} else {
				boss.facing = -1.0f;
			}
		}

		if (game.debug) {
			if (game.key_pressed[SDL_SCANCODE_B]) {
				EndBossPhase();

				if (boss_exists) {
					//if (!IsKeyDown(KEY_LEFT_CONTROL)) {
						if (boss.state == BossState::WaitingEnd) {
							boss.wait_timer = 0.0f;
						}
					//}
				}
			}
		}
	}

	template <typename Object>
	void Stage::DrawObject(SDL_Renderer* renderer, Object& object, float angle, float xscale, float yscale, SDL_Color color) {
		float x = object.x;
		float y = object.y;
		SpriteData* sprite = object.sc.sprite;
		int frame_index = (int)object.sc.frame_index;

		x += screen_shake_x;
		y += screen_shake_y;

		DrawSprite(renderer, sprite, frame_index, x, y, angle, xscale, yscale, color);
	}

	void Stage::Draw(SDL_Renderer* renderer, SDL_Texture* target, float delta) {
		SDL_SetRenderTarget(renderer, target);
		{
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);

			// stage bg
			{
				StageData* stage = GetStageData(game.stage_index);
				if (stage->draw) {
					(*stage->draw)(&game, renderer, stage_memory, spellcard_bg_alpha < 1.0f, delta);
				}
			}

			// spell card bg
			if (spellcard_bg_alpha > 0.0f) {
				SDL_Texture* texture = game.assets.GetTexture("CirnoSpellcardBG.png");
				SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear);
				SDL_SetTextureAlphaMod(texture, (unsigned char)(255.0f * spellcard_bg_alpha));
				{
					SDL_Rect dest{0, (int)time / 5 % PLAY_AREA_W - PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
					SDL_RenderCopy(renderer, texture, nullptr, &dest);
				}
				{
					SDL_Rect dest{0, (int)time / 5 % PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
					SDL_RenderCopy(renderer, texture, nullptr, &dest);
				}
				{
					SDL_Rect dest{0, (int)time / 5 % PLAY_AREA_W + PLAY_AREA_W, PLAY_AREA_W, PLAY_AREA_W};
					SDL_RenderCopy(renderer, texture, nullptr, &dest);
				}
			}

			for (Enemy& enemy : enemies) {
				DrawObject(renderer, enemy, enemy.angle);
			}

			if (boss_exists) {
				DrawObject(renderer, boss, 0.0f, -boss.facing, 1.0f);
			}

			// draw player
			{
				SDL_Color color = {255, 255, 255, 255};
				float xscale = 1.0f;
				float yscale = 1.0f;

				if (player.state == PlayerState::Dying || player.state == PlayerState::Appearing) {
					float f;
					if (player.state == PlayerState::Dying) {
						f = 1.0f - player.timer / PLAYER_DEATH_TIME;
					} else {
						f = player.timer / PLAYER_APPEAR_TIME;
					}
					//f = std::min(f * 2.0f, 1.0f);

					xscale = cpml::lerp(1.0f, 0.25f, f);
					yscale = cpml::lerp(1.0f, 2.0f, f);
					color.a = (unsigned char)cpml::lerp(255.0f, 0.0f, f);
				} else {
					if (player.iframes > 0.0f) {
						if (((int)time / 4) % 2) {
							//color.a /= 2;
							color = {128, 128, 128, 128};
						}
					}
				}

				DrawObject(renderer, player, 0.0f, -player.facing * xscale, yscale, color);

				if (player.hitbox_alpha > 0.0f) {
					SpriteData* sprite = game.assets.GetSprite("Hitbox");
					unsigned char a = (unsigned char) (255.0f * player.hitbox_alpha);
					float x = player.x + screen_shake_x;
					float y = player.y + screen_shake_y;
					DrawSprite(renderer, sprite, 0, x, y, -time, 1.0f, 1.0f, {255, 255, 255, a});
				}
			}

			for (Pickup& pickup : pickups) {
				DrawObject(renderer, pickup);
			}

			for (PlayerBullet& b : player_bullets) {
				SDL_Color color = {255, 255, 255, 80};
				switch (b.type) {
					case PLAYER_BULLET_REIMU_CARD: {
						DrawObject(renderer, b, b.rotation, 1.5f, 1.5f, color);
						break;
					}
					case PLAYER_BULLET_REIMU_ORB_SHOT: {
						DrawObject(renderer, b, b.dir, 1.5f, 1.5f, color);
						break;
					}
				}
			}

			for (Bullet& bullet : bullets) {
				switch (bullet.type) {
					case ProjectileType::Lazer: {
						float angle = bullet.dir + 90.0f;
						float xscale = (bullet.thickness + 2.0f) / 16.0f;
						float yscale = bullet.length / 16.0f;
						DrawObject(renderer, bullet, angle, xscale, yscale);
						break;
					}
					case ProjectileType::SLazer: {
						float angle = bullet.dir + 90.0f;
						float xscale = (bullet.thickness + 2.0f) / 16.0f;
						float yscale = bullet.target_length / 16.0f;
						if (bullet.lazer_timer < bullet.lazer_time) {
							xscale = 2.0f / 16.0f;
						}
						DrawObject(renderer, bullet, angle, xscale, yscale);
						break;
					}
					default: {
						float angle = 0.0f;
						if (bullet.rotate) {
							angle = bullet.dir - 90.0f;
						}
						DrawObject(renderer, bullet, angle);
						break;
					}
				}
			}

			// GUI
			{
				// pickup labels
				for (Pickup& pickup : pickups) {
					if (pickup.y < 0.0f) {
						SpriteData* sprite = pickup.sc.sprite;
						int frame_index = pickup.type + PICKUP_COUNT;
						float x = pickup.x;
						float y = 8.0f;
						SDL_Color color{255, 255, 255, 192};
						DrawSprite(renderer, sprite, frame_index, x, y, 0.0f, 1.0f, 1.0f, color);
					}
				}

				if (boss_exists) {
					BossData* data = GetBossData(boss.type_index);
					PhaseData* phase = GetPhaseData(data, boss.phase_index);

					// phases left
					{
						//char buf[3];
						//stbsp_snprintf(buf, sizeof(buf), "%2d", data->phase_count - boss.phase_index - 1);
						char buf[2] = {'0' + data->phase_count - boss.phase_index - 1, 0};
						int x = 8;
						int y = 0;
						DrawTextBitmap(renderer, game.assets.fntMain, buf, x, y);
					}

					// healthbar
					{
						int healthbar_x = 32 + 2;
						int healthbar_y = 6;
						int healthbar_w = PLAY_AREA_W - 64 - 4;
						int healthbar_h = 2;
						int reduced_w = (int) ((float)healthbar_w * (boss.hp / phase->hp));
						{
							SDL_Rect rect{healthbar_x, healthbar_y + 1, reduced_w, healthbar_h};
							SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
							SDL_RenderFillRect(renderer, &rect);
						}
						{
							SDL_Rect rect{healthbar_x, healthbar_y, reduced_w, healthbar_h};
							SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
							SDL_RenderFillRect(renderer, &rect);
						}
					}

					// timer
					{
						char buf[3];
						stbsp_snprintf(buf, sizeof(buf), "%d", (int)(boss.timer / 60.0f));
						int x = PLAY_AREA_W - 2 * 15;
						int y = 0;
						if ((int)(boss.timer / 60.0f) < 10) x += 8;
						DrawTextBitmap(renderer, game.assets.fntMain, buf, x, y);
					}

					// boss name
					{
						SDL_Surface* surface = TTF_RenderText_Blended(game.assets.fntCirno, data->name, {255, 255, 255, 255});
						SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
						{
							SDL_SetTextureColorMod(texture, 0, 0, 0);
							SDL_Rect dest{17, 17, surface->w, surface->h};
							SDL_RenderCopy(renderer, texture, nullptr, &dest);
						}
						{
							SDL_SetTextureColorMod(texture, 255, 255, 255);
							SDL_Rect dest{16, 16, surface->w, surface->h};
							SDL_RenderCopy(renderer, texture, nullptr, &dest);
						}
						SDL_DestroyTexture(texture);
						SDL_FreeSurface(surface);
					}

					// phase name
					if (phase->type == PHASE_SPELLCARD && boss.state != BossState::WaitingEnd) {
						SDL_Surface* surface = TTF_RenderText_Blended(game.assets.fntCirno, phase->name, {255, 255, 255, 255});
						SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
						{
							SDL_SetTextureColorMod(texture, 0, 0, 0);
							SDL_Rect dest{PLAY_AREA_W - 15 - surface->w, 17, surface->w, surface->h};
							SDL_RenderCopy(renderer, texture, nullptr, &dest);
						}
						{
							SDL_SetTextureColorMod(texture, 255, 255, 255);
							SDL_Rect dest{PLAY_AREA_W - 16 - surface->w, 16, surface->w, surface->h};
							SDL_RenderCopy(renderer, texture, nullptr, &dest);
						}
						SDL_DestroyTexture(texture);
						SDL_FreeSurface(surface);
					}
				}
			}
		}
		SDL_SetRenderTarget(renderer, nullptr);
	}

	Enemy& Stage::CreateEnemy() {
		Enemy& enemy = enemies.emplace_back();
		enemy.id = (next_id++) | (TYPE_ENEMY << TYPE_PART_SHIFT);
		return enemy;
	}

	Bullet& Stage::CreateBullet() {
		Bullet& bullet = bullets.emplace_back();
		bullet.id = (next_id++) | (TYPE_BULLET << TYPE_PART_SHIFT);
		return bullet;
	}

	Player& Stage::CreatePlayer(bool from_death) {
		CharacterData* character = GetCharacterData(game.player_character);

		player = {};
		player.id = 0 | (TYPE_PLAYER << TYPE_PART_SHIFT);
		player.x = PLAYER_STARTING_X;
		player.y = PLAYER_STARTING_Y;
		player.radius = character->radius;
		player.sc.sprite = character->idle_spr;

		player.iframes = PLAYER_RESPAWN_IFRAMES;
		if (from_death) {
			player.state = PlayerState::Appearing;
			player.timer = PLAYER_APPEAR_TIME;
		}

		return player;
	}

	Boss& Stage::CreateBoss() {
		if (boss_exists) {
			FreeBoss();
		}
		boss = {};
		boss.id = 0 | (TYPE_BOSS << TYPE_PART_SHIFT);
		boss_exists = true;
		return boss;
	}

	Pickup& Stage::CreatePickup(float x, float y, unsigned char type) {
		Pickup& pickup = pickups.emplace_back();
		pickup.x = x;
		pickup.y = y;
		pickup.vsp = -1.5f;
		pickup.radius = 8.0f;
		pickup.sc.sprite = game.assets.GetSprite("Pickup");
		pickup.sc.frame_index = (float)type;
		pickup.type = type;
		return pickup;
	}

	PlayerBullet& Stage::CreatePlayerBullet() {
		PlayerBullet& player_bullet = player_bullets.emplace_back();
		return player_bullet;
	}

	typedef ptrdiff_t ssize;

	template <typename T>
	static T* BinarySearch(std::vector<T>& storage, instance_id id) {
		ssize left = 0;
		ssize right = (ssize)storage.size() - 1;

		while (left <= right) {
			ssize middle = (left + right) / 2;
			if (storage[middle].id < id) {
				left = middle + 1;
			} else if (storage[middle].id > id) {
				right = middle - 1;
			} else {
				return &storage[middle];
			}
		}

		return nullptr;
	}

	Enemy* Stage::FindEnemy(instance_id id) {
		return BinarySearch(enemies, id);
	}

	Bullet* Stage::FindBullet(instance_id id) {
		return BinarySearch(bullets, id);
	}

	Player* Stage::FindPlayer(instance_id id) {
		Player* result = nullptr;
		if (player.id == id) {
			result = &player;
		}
		return result;
	}

	Boss* Stage::FindBoss(instance_id id) {
		Boss* result = nullptr;
		if (boss_exists) {
			if (boss.id == id) {
				result = &boss;
			}
		}
		return result;
	}

	void Stage::FreeEnemy(Enemy& enemy) {
		if (enemy.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.coroutine);
		if (enemy.update_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.update_callback);
		if (enemy.death_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, enemy.death_callback);
	}

	void Stage::FreeBullet(Bullet& bullet) {
		if (bullet.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, bullet.coroutine);
		if (bullet.update_callback != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, bullet.update_callback);
	}

	void Stage::FreeBoss() {
		if (boss.coroutine != LUA_REFNIL) luaL_unref(L, LUA_REGISTRYINDEX, boss.coroutine);
	}

}
