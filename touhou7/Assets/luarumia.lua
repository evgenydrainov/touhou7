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
		SLazer{GetX(id), GetY(id), TargetDir(id), seconds(1), seconds(1), 10, 2}

		wait(seconds(4))
	end
end
