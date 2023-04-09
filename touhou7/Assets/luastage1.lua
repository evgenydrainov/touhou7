local sprEnemy0 = FindSprite("Enemy0");
local sprFairy0 = FindSprite("Fairy0");

local spawn_spinner = function()
	local Script = function(id)
		wait(60);
		SetAcc(id, -0.01);
		while (GetSpd(id) > 0) do wait(1); end
		SetDir(id, -GetDir(id));
		SetAcc(id, 0.01);
	end;

	local OnDeath = function(id)
		local r = choose(1, 2, 3);
		if (r == 1) then
			local dir = random(360);
			local x = GetX(id);
			local y = GetY(id);
			Shoot{x, y, 2, dir,       0, BULLET_RICE, 2};
			Shoot{x, y, 2, dir + 90,  0, BULLET_RICE, 2};
			Shoot{x, y, 2, dir + 180, 0, BULLET_RICE, 2};
			Shoot{x, y, 2, dir + 270, 0, BULLET_RICE, 2};
		elseif (r == 2) then
			local dir = random(360);
			local x = GetX(id);
			local y = GetY(id);
			Shoot{x, y, 3, dir,       0, BULLET_KUNAI, 10};
			Shoot{x, y, 3, dir + 90,  0, BULLET_KUNAI, 10};
			Shoot{x, y, 3, dir + 180, 0, BULLET_KUNAI, 10};
			Shoot{x, y, 3, dir + 270, 0, BULLET_KUNAI, 10};
		end
	end;

	local OnUpdate = function(id)
		SetAngle(id, GetAngle(id) + 10);
	end;

	local x = random(0, PLAY_AREA_W);
	local y = 0;
	CreateEnemy{x, y, 2, 270 + random(-30, 30), 0, sprEnemy0, PICKUP_POINT|PICKUP_POWER, Script, OnDeath, OnUpdate};
end;

function Stage1_Script(id)
	-- print_table(_ENV);

	-- print(collectgarbage("count"));

	wait(60);

	if (SKIP_TO_BOSS) then
		goto L_Boss;
	end

	if (SKIP_TO_MIDBOSS) then
		goto L_Midboss;
	end

	-- stage part
	do
		for i = 1, 50 do
			spawn_spinner();
			wait(5);
		end

		wait(180);

		for j = 1, 4 do
			for i = 1, 18 do
				local x = i * (PLAY_AREA_W / 19);
				if (j % 2 == 0) then
					x = PLAY_AREA_W - x;
				end
				local y = 0;

				local target = GetTarget(id);
				local target_x = GetX(target);
				local target_y = GetY(target);
				CreateEnemy{x, y, 2, point_direction(x, y, target_x, target_y), 0, sprFairy0, PICKUP_POINT|PICKUP_POWER|PICKUP_NOTHING};

				if (i >= 12) then
					spawn_spinner();
				end

				wait(20);
			end

			wait(60);
		end

		wait(180);
	end

	-- midboss part
::L_Midboss::
	do
		local dai = CreateBoss{type=BOSS_DAIYOUSEI};
		while (Exists(dai)) do wait(1); end

		wait(60);
	end

	-- boss part
::L_Boss::
	do
		CreateBoss{type=BOSS_CIRNO};
	end
end

function Daiyousei_Nonspell1(id)
	local tp = function()
		local x = random(32, PLAY_AREA_W - 32);
		local y = random(32, BOSS_STARTING_Y * 2 - 32);
		SetX(id, x);
		SetY(id, y);
	end;

	--local t = {GetX(id), GetY(id), 3, TargetDir(id), 0, BULLET_KUNAI, 2};

	while (true) do
		for j = 1, 2 do
			local n = 48;
			for i = 0, n - 1 do
				local x = GetX(id);
				local y = GetY(id);
				local dir;
				local color;
				if (j == 1) then
					dir = lerp(270 + 360, 270, i / n);
					color = 10;
				else
					dir = lerp(270, 270 + 360, i / n);
					color = 2;
				end
				local spd = lerp(1.5, 3, i / n);
				local a = lerp(1.1, 1.5, i / n);
				Shoot{x, y, spd,     dir, 0, BULLET_KUNAI, color};
				Shoot{x, y, spd * a, dir, 0, BULLET_KUNAI, color};

				-- Shoot{x, y, 0, dir, 0, BULLET_KUNAI, color, BulletTransformation(5, 1, 30)};
				-- Shoot{x, y, 0, dir, 0, BULLET_KUNAI, color, function(id)
				-- 	BulletTransformation(10, 2, 15)(id);
				-- 	SetDir(id, TargetDir(id));
				-- end};

				-- Shoot(t);

				wait(1);
			end

			wait(100);

			tp();

			wait(100);
		end

		for i = 1, 20 do
			ShootRadial(3, 40, function()
				return Shoot{GetX(id), GetY(id), 2.25, TargetDir(id), 0, BULLET_PELLET, 6};
			end);

			ShootRadial(3, 40, function()
				local script = function(id)
					wait(30);
					SetSpd(id, 4);
					SetAcc(id, 0);
					SetDir(id, TargetDir(id));
				end;

				return {
					Shoot{GetX(id), GetY(id), 4,       TargetDir(id), -0.1, BULLET_PELLET, 15, script},
					Shoot{GetX(id), GetY(id), 4 * 1.2, TargetDir(id), -0.1, BULLET_PELLET, 15, script}
				};
			end);

			wait(10);
		end

		wait(100);

		tp();

		wait(100);
	end
end
