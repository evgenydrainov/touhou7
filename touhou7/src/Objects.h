#pragma once

#include "Assets.h"

#include <lua.hpp>

#define TYPE_PART_SHIFT 28
#define ID_PART_MASK 0x0FFF'FFFF

namespace th {

	enum ObjectType : unsigned char {
		TYPE_BULLET,
		TYPE_ENEMY,
		TYPE_PLAYER,
		TYPE_BOSS
	};

	typedef unsigned int instance_id;

	struct SpriteComponent {
		SpriteData* sprite;
		float frame_index;
	};

	enum class PlayerState : unsigned char {
		Normal,
		Dying,
		Appearing
	};

	struct ReimuData {
		float fire_timer;
		int fire_queue;
		float orb_x[2];
		float orb_y[2];
	};

	struct MarisaData {

	};

	struct Player {
		instance_id id;
		bool dead;

		float x;
		float y;
		float hsp;
		float vsp;
		float radius;
		SpriteComponent sc;
		PlayerState state;
		bool focus;
		float iframes;
		float timer;
		float bomb_timer;
		float hitbox_alpha;
		float facing = 1.0f;

		union {
			ReimuData reimu;
			MarisaData marisa;
		};
	};

	enum {
		PLAYER_BULLET_REIMU_CARD,
		PLAYER_BULLET_REIMU_ORB_SHOT
	};

	struct PlayerBullet {
		float x;
		float y;
		float spd;
		float dir;
		float acc;
		float radius;
		SpriteComponent sc;
		float dmg;
		unsigned char type;
		union {
			float rotation;
		};
	};

	enum class ProjectileType : unsigned char {
		Bullet,
		Rect,
		Lazer,
		SLazer
	};

	struct Bullet {
		instance_id id;
		bool dead;

		float x;
		float y;
		float spd;
		float dir;
		float acc;

		ProjectileType type;
		union {
			struct {
				float radius;
				bool rotate;
			};
			struct {
				float length;
				float thickness;
				float target_length;
				float lazer_time;
				float lazer_timer;
				float lazer_lifetime;
			};
		};

		SpriteComponent sc;
		float lifetime;
		bool grazed;

		int coroutine = LUA_REFNIL;
		int update_callback = LUA_REFNIL;
	};

	struct Enemy {
		instance_id id;
		bool dead;

		float x;
		float y;
		float spd;
		float dir;
		float acc;
		float radius;
		SpriteComponent sc;
		float angle;
		float hp;
		unsigned int drops;

		int coroutine = LUA_REFNIL;
		int update_callback = LUA_REFNIL;
		int death_callback = LUA_REFNIL;
	};

	enum class BossState : unsigned char {
		Normal,
		WaitingStart,
		WaitingEnd
	};

	struct Boss {
		instance_id id;
		bool dead;

		float x;
		float y;
		float spd;
		float dir;
		float acc;
		float radius;
		SpriteComponent sc;
		float hp;
		int type_index;
		int phase_index;
		float timer;
		float wait_timer;
		BossState state;
		float facing = 1.0f;

		int coroutine = LUA_REFNIL;
	};

	enum {
		PICKUP_POWER,
		PICKUP_POINT,
		PICKUP_BIGP,
		PICKUP_BOMB,
		PICKUP_FULL_POWER,
		PICKUP_1UP,
		PICKUP_SCORE,
		PICKUP_CHERRY,

		PICKUP_COUNT
	};

	struct Pickup {
		float x;
		float y;
		float hsp;
		float vsp;
		float radius;
		SpriteComponent sc;
		unsigned char type;
		bool homing;
	};

}
