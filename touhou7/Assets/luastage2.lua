local function WaitTillDies(obj)
	while Exists(obj) do
		wait(1)
	end
end

function Stage2_Script(id)
	wait(60)

	WaitTillDies(CreateBoss{type=BOSS_RUMIA})

	wait(60)
end
