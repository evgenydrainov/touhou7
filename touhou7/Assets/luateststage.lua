function TestStage_Script1(id)
	print "test stage\n"
	wait(120)

	-- goto _BULLET_KILLS_OTHER_BULLETS

::KILLING_PLAYER::
	print "killing player...\n"
	Destroy(GetTarget(id))
	wait(120)

::KILLING_BOSS::
	do
		local boss = CreateBoss{type=0}
		wait(120)

		print "killing boss...\n"
		Destroy(boss)
		wait(120)
	end

::_BULLET_SPAWNING_OTHER_BULLETS::
	do
		print "bullet spawning other bullets"
		local bullet = Shoot{x=PLAY_AREA_W/2, y=0, spd=1, dir=270, acc=0, type=BULLET_BULLET, color=0, Script=function(id)
			while true do
				Shoot{x=GetX(id), y=GetY(id), spd=4, dir=0, acc=0, type=BULLET_KUNAI, color=2}
				Shoot{x=GetX(id), y=GetY(id), spd=4, dir=180, acc=0, type=BULLET_KUNAI, color=2}
				wait(1)
			end
		end}
		while Exists(bullet) do wait(1) end
		print "done\n"
		wait(120)
	end

::_BULLET_KILLS_ITSELF_FROM_COROUTINE::
	do
		print "bullet kills itself from coroutine"
		local bullet = Shoot{x=PLAY_AREA_W/2, y=0, spd=2, dir=270, acc=0, type=BULLET_BULLET, color=2, Script=function(id)
			wait(120)
			Destroy(id)
		end}
		while Exists(bullet) do wait(1) end
		print "done\n"
		wait(120)
	end

::_BULLET_KILLS_OTHER_BULLETS::
	do
		print "bullet kills other bullets"
		local bullets = {}
		local t=0
		for i = 1, 1000 do
			bullets[i] = Shoot{x=PLAY_AREA_W/2, y=PLAY_AREA_H/2, spd=0.5, dir=random(0,360), acc=0, type=BULLET_PELLET, color=choose(6,10)}
			t=t+0.1
			if t>=1 then wait(1) t=t-1 end
		end
		wait(10)
		local bullet = Shoot{x=PLAY_AREA_W/2, y=0, spd=2, dir=270, acc=0, type=BULLET_BULLET, color=0, Script=function(id)
			local t=0
			for i = 1, 1000 do
				Destroy(bullets[i])
				t=t+0.1
				if t>=1 then wait(1) t=t-1 end
			end
		end}
		while Exists(bullet) do wait(1) end
		print "done\n"
		wait(120)
	end



	print "tests ended"
end

function TestStage_Script(id)
	wait(60)

	CreateBoss{type=BOSS_RUMIA}

	wait(60)
end
