#include "Game.h"

namespace th {

	// characters
	void ReimuShotType(Game* ctx, float delta);
	void ReimuBomb(Game* ctx);

	static_assert(CHARACTER_COUNT == 1);
	CharacterData character_data[CHARACTER_COUNT] = {
		{"Reimu Hakurei", 3.75f, 1.6f, 2.0f, 16.0f, 15.0f, 2, ReimuShotType, ReimuBomb}
	};

	// bullet data
	static_assert(BULLET_TYPE_COUNT == 7);
	BulletData bullet_data[BULLET_TYPE_COUNT] = {
		{nullptr, 3.0f, true},
		{nullptr, 3.0f, false},
		{nullptr, 4.0f, false},
		{nullptr, 2.0f, true},
		{nullptr, 2.0f, true},
		{nullptr, 2.0f, true},
		{nullptr, 2.0f, false}
	};

	// stages
	void Stage1_Init(Game* ctx, SDL_Renderer* renderer, void* mem);
	void Stage1_Quit(Game* ctx, void* mem);
	void Stage1_Update(Game* ctx, void* mem, bool visible, float delta);
	void Stage1_Draw(Game* ctx, SDL_Renderer* renderer, void* mem, bool visible, float delta);

	static_assert(STAGE_COUNT == 2);
	StageData stage_data[STAGE_COUNT] = {
		{"Stage1_Script", Stage1_Init, Stage1_Quit, Stage1_Update, Stage1_Draw},
		{"Stage2_Script"}
	};

	// bosses
	static_assert(BOSS_TYPE_COUNT == 3);
	BossData boss_data[BOSS_TYPE_COUNT] = {
		{
			"Daiyousei",
			1,
			nullptr,
			BOSS_MIDBOSS,
			{
				{1000.0f, 31.0f * 60.0f, PHASE_NONSPELL, "Daiyousei_Nonspell1"}
			}
		},

		{
			"Cirno",
			5,
			nullptr,
			BOSS_BOSS,
			{
				{1500.0f, 25.0f * 60.0f, PHASE_NONSPELL, "Cirno_Nonspell1"},
				{1500.0f, 30.0f * 60.0f, PHASE_SPELLCARD, "Cirno_IcicleFall", "Ice Sign \"Icicle Fall\""},
				{1500.0f, 50.0f * 60.0f, PHASE_NONSPELL, "Cirno_Nonspell2"},
				{1500.0f, 40.0f * 60.0f, PHASE_SPELLCARD, "Cirno_PerfectFreeze", "Freeze Sign \"Perfect Freeze\""},
				{1500.0f, 33.0f * 60.0f, PHASE_SPELLCARD, "Cirno_DiamondBlizzard", "Snow Sign \"Diamond Blizzard\""}
			}
		},

		{
			"Rumia",
			3,
			nullptr,
			BOSS_BOSS,
			{
				{1000.0f, 30.0f * 60.0f, PHASE_NONSPELL, "Rumia_Nonspell1"},
				{1000.0f, 30.0f * 60.0f, PHASE_SPELLCARD, "Rumia_NightBird", "Night Sign \"Night Bird\""},
				{1000.0f, 30.0f * 60.0f, PHASE_NONSPELL, "Rumia_Nonspell2"}
			}
		}
	};

	void Game::FillDataTables() {
		// characters
		static_assert(CHARACTER_COUNT == 1);
		character_data[CHARACTER_REIMU].idle_spr = assets.GetSprite("ReimuIdle");
		character_data[CHARACTER_REIMU].turn_spr = assets.GetSprite("ReimuTurn");

		// bullet data
		static_assert(BULLET_TYPE_COUNT == 7);
		bullet_data[0].sprite = assets.GetSprite("Bullet0");
		bullet_data[1].sprite = assets.GetSprite("Bullet1");
		bullet_data[2].sprite = assets.GetSprite("Bullet2");
		bullet_data[3].sprite = assets.GetSprite("Bullet3");
		bullet_data[4].sprite = assets.GetSprite("Bullet4");
		bullet_data[5].sprite = assets.GetSprite("Bullet5");
		bullet_data[6].sprite = assets.GetSprite("Bullet6");

		// bosses
		static_assert(BOSS_TYPE_COUNT == 3);
		boss_data[0].sprite = assets.GetSprite("Daiyousei");
		boss_data[1].sprite = assets.GetSprite("CirnoIdle");
		boss_data[2].sprite = assets.GetSprite("RumiaIdle");
	}

	// test stages
#define TEST_STAGE_COUNT 1
	static StageData test_stage_data[TEST_STAGE_COUNT] = {
		{"TestStage_Script"}
	};

	StageData* GetStageData(int stage_index) {
		if (0 <= stage_index && stage_index < STAGE_COUNT) {
			return &stage_data[stage_index];
		}
		if (stage_index >= 100 && stage_index - 100 < TEST_STAGE_COUNT) {
			return &test_stage_data[stage_index - 100];
		}
		return &stage_data[0];
	}

	BulletData* GetBulletData(int type_index) {
		if (0 <= type_index && type_index < BULLET_TYPE_COUNT) {
			return &bullet_data[type_index];
		}
		return &bullet_data[0];
	}

	CharacterData* GetCharacterData(int character_index) {
		if (0 <= character_index && character_index < CHARACTER_COUNT) {
			return &character_data[character_index];
		}
		return &character_data[0];
	}

	BossData* GetBossData(int type_index) {
		if (0 <= type_index && type_index < BOSS_TYPE_COUNT) {
			return &boss_data[type_index];
		}
		return &boss_data[0];
	}

	PhaseData* GetPhaseData(BossData* boss_data, int phase_index) {
		if (!boss_data) {
			boss_data = GetBossData(0);
		}
		if (0 <= phase_index && phase_index < boss_data->phase_count) {
			return &boss_data->phase_data[phase_index];
		}
		return &boss_data->phase_data[0];
	}

}
