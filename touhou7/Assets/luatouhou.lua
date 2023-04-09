math.random = nil;
math.randomseed = nil;

sin = math.sin;
cos = math.cos;
rad = math.rad;
deg = math.deg;

function dsin(x) return sin(rad(x)); end
function dcos(x) return cos(rad(x)); end

function lengthdir_x(l, d) return l *  dcos(d); end
function lengthdir_y(l, d) return l * -dsin(d); end

min = math.min;
max = math.max;

function clamp(x, _min, _max) return min(max(x, _min), _max); end

function approach(a, b, n) return a + clamp(b - a, -n, n); end

function sqr(x) return x * x; end

sqrt = math.sqrt;

function point_direction(x1, y1, x2, y2) return deg(math.atan(y1 - y2, x2 - x1));  end
function point_distance(x1, y1, x2, y2)  return sqrt(sqr(x2 - x1) + sqr(y2 - y1)); end

function lerp(a, b, f) return a + (b - a) * f; end

function round(x) return x + (2^52 + 2^51) - (2^52 + 2^51); end

function wrap(a, b) return (a % b + b) % b; end

abs   = math.abs;
floor = math.floor;
ceil  = math.ceil;

function choose(...)
	local arg = {...};
	local i = 1 + floor(random(#arg));
	return arg[i];
end

function ivalues(t)
	local i = 0;
	return function() i = i + 1; return t[i]; end
end

function range(n)
	local i = 0;
	return function() i = i + 1; if (i <= n) then return i; end end
end

function wait(t)
	while (t > 0) do
		coroutine.yield();
		t = t - 1;
	end
end

function ShootRadial(n, dir_diff, f)
	local res = {};
	for i = 0, n - 1 do
		local mul = -(n - 1) / 2 + i;
		local got = f(i);
		if (type(got) == "table") then
			for b in ivalues(got) do
				SetDir(b, GetDir(b) + dir_diff * mul);
				res[#res + 1] = b;
			end
		elseif (type(got) == "number") then
			SetDir(got, GetDir(got) + dir_diff * mul);
			res[#res + 1] = got;
		end
	end
	return res;
end

function EntityDir(e1, e2)
	return point_direction(GetX(e1), GetY(e1), GetX(e2), GetY(e2));
end

function TargetDir(id)
	return EntityDir(id, GetTarget(id));
end

function LaunchTowardsPoint(id, target_x, target_y, acc)
	acc = abs(acc);
	local x = GetX(id);
	local y = GetY(id);
	local dist = point_distance(x, y, target_x, target_y);
	SetSpd(id, sqrt(dist * acc * 2));
	SetAcc(id, -acc);
	SetDir(id, point_direction(x, y, target_x, target_y));
end

PLAY_AREA_W     = 384;
PLAY_AREA_H     = 448;
BOSS_STARTING_X = 192;
BOSS_STARTING_Y = 96;

BULLET_BULLET = 0;
BULLET_OUT    = 1;
BULLET_FILL   = 2;
BULLET_RICE   = 3;
BULLET_KUNAI  = 4;
BULLET_PELLET = 5;
BULLET_SMALL  = 6;

PICKUP_POWER	  = 1;
PICKUP_POINT	  = 1 << 1;
PICKUP_BIGP		  = 1 << 2;
PICKUP_BOMB		  = 1 << 3;
PICKUP_FULL_POWER = 1 << 4;
PICKUP_1UP		  = 1 << 5;
PICKUP_SCORE	  = 1 << 6;
PICKUP_CHERRY	  = 1 << 7;
PICKUP_NOTHING	  = 1 << 31;

function print_table(t)
	for k, v in pairs(t) do
		print(tostring(k) .. " = " .. tostring(v));
	end
end

function BulletTransformation(spd_from, spd_to, time)
	local timer = 0;
	return function(id)
		SetSpd(id, spd_from)
		while (timer < time) do
			SetSpd(id, spd_from + (spd_to - spd_from) * (timer / time));
			wait(1);
			timer = timer + 1;
		end
		SetSpd(id, spd_to);
	end;
end

BOSS_DAIYOUSEI	= 0;
BOSS_CIRNO		= 1;
BOSS_RUMIA		= 2;

function Wander(id)
	local target_x = random(32, PLAY_AREA_W-32)
	local target_y = random(32, BOSS_STARTING_Y*2-32)
	local x = GetX(id)
	local y = GetY(id)
	target_x = clamp(target_x, x-80, x+80)
	target_y = clamp(target_y, y-80, y+80)
	LaunchTowardsPoint(id, target_x, target_y, 0.01)
end

function GoBack(id)
	LaunchTowardsPoint(id, BOSS_STARTING_X, BOSS_STARTING_Y, 0.02)
end
