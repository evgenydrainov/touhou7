local Lazer = ShootLazer
local SLazer = ShootSLazer
local Radial = ShootRadial

local function seconds(t) return t*60 end

function Rumia_Nonspell1(id)
	while true do
		Wander(id)
		wait(10)

		for j = 1, 7 do
			Radial(10, 0, function(i)
				return Shoot{GetX(id), GetY(id), lerp(1.5,5,i/9), TargetDir(id), 0, BULLET_FILLED, 1+j%2}
			end)

			wait(10)
		end

		wait(60)

		Wander(id)

		wait(60)

		-- red rice
		Radial(16, 0, function(j)
			return Radial(3, 4, function()
				return Shoot{GetX(id), GetY(id), lerp(1.5,5,j/15), TargetDir(id), 0, BULLET_RICE, 1}
			end)
		end)

		wait(60)

		-- blue
		Radial(16, 360/16, function()
			return Shoot{GetX(id), GetY(id), 2.5, TargetDir(id), 0, BULLET_OUTLINE, 6}
		end)

		wait(200)
	end
end

function Rumia_NightBird(id)
	while true do
		for i = 1, 4 do
			for i = 0, 15 do
				local dir = TargetDir(id)
				local off = 30
				Shoot{GetX(id), GetY(id), lerp(1, 2.5, i/15), lerp(dir+90-off, dir-90-off, i/15), 0, BULLET_OUTLINE, 6}
				wait(2)
			end

			for i = 0, 15 do
				local dir = TargetDir(id) + 5
				local off = 30
				Shoot{GetX(id), GetY(id), lerp(1, 2.5, i/15), lerp(dir-90+off, dir+90+off, i/15), 0, BULLET_OUTLINE, 8}
				wait(2)
			end
		end

		wait(120)

		Wander(id)
	end
end

function Rumia_Nonspell2(id)
	while true do
		Wander(id)

		wait(10)

		Radial(8, 0, function(j)
			return Radial(2, 5, function()
				return Shoot{GetX(id), GetY(id), lerp(1.75, 3.25, j/7), TargetDir(id), 0, BULLET_OUTLINE, 10}
			end)
		end)

		for i = 1, 6 do
			SLazer{GetX(id), GetY(id), TargetDir(id), seconds(2), seconds(1), 10, 8}

			wait(10)
		end

		wait(seconds(2.5))

		Wander(id)

		for i = 0, 32 do
			local spd = lerp(0.5, 4, i/32)
			if i%2==0 then spd = spd*1.1 end
			Shoot{GetX(id), GetY(id), spd, lerp(0, -225, i/32), 0, BULLET_OUTLINE, 10}
			wait(1)
		end

		wait(seconds(1))

		Wander(id)

		wait(seconds(1))

		for i = 0, 2 do
			Radial(8, lerp(10, 20, i/2), function()
				return Shoot{GetX(id), GetY(id), 3.25, TargetDir(id), 0, BULLET_RICE, 13}
			end)

			Radial(9, lerp(10, 20, i/2), function()
				return Shoot{GetX(id), GetY(id), 2, TargetDir(id), 0, BULLET_RICE, 13}
			end)

			wait(40)
		end

		wait(seconds(1))

		Wander(id)

		wait(seconds(1))

		Radial(39, 360/39, function() return Shoot{GetX(id), GetY(id), 2, 0, 0, BULLET_RICE, 0} end)
		wait(40)
		Radial(38, 360/38, function() return Shoot{GetX(id), GetY(id), 4, 0, 0, BULLET_SMALL, 0} end)
		wait(40)
		Radial(39, 360/39, function() return Shoot{GetX(id), GetY(id), 2, 0, 0, BULLET_RICE, 0} end)
		wait(40)

		wait(120)
	end
end

function Rumia_Demarcation(id)
	local function f(color)
		Radial(20, 360/20, function()
			return Shoot{GetX(id), GetY(id), 4, 0, -0.08, BULLET_RICE, color, function(id)
				while GetSpd(id)>0 do wait(1) end
				SetDir(id, GetDir(id) + 90)
				SetSpd(id, 2)
				SetAcc(id, 0)
			end}
		end)

		Radial(20, 360/20, function()
			return Shoot{GetX(id), GetY(id), 4, 10, -0.08, BULLET_RICE, color, function(id)
				while GetSpd(id)>0 do wait(1) end
				SetDir(id, GetDir(id) - 90)
				SetSpd(id, 2)
				SetAcc(id, 0)
			end}
		end)
	end

	while true do
		f(6)
		wait(seconds(1))

		f(10)
		wait(seconds(1))

		f(2)
		wait(seconds(1))

		Wander(id)
		wait(10)

		for i = 1, 4 do
			Radial(3, 0, function(j)
				wait(5)
				return Radial(10+j, 10, function()
					return Shoot{GetX(id), GetY(id), lerp(5, 5.5, j/2), TargetDir(id), -0.2, BULLET_OUTLINE, 6, function(id)
						wait(30)
						SetSpd(id, 4)
						SetDir(id, TargetDir(id))
						SetAcc(id, 0)
					end}
				end)
			end)
			wait(10)
		end

		wait(seconds(1))
	end
end
