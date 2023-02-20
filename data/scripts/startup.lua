-- Copyright(c) 2022-2023, KaoruXun All rights reserved.
-- runs on start of the engine
-- used to load scripts
tl = require("tl")
inspect = require("inspect")
sandbox = require("sandbox")
lang = require("lang")

assert(inspect({ 1, 2, 3, 4 }) == "{ 1, 2, 3, 4 }")
assert(inspect(1) == "1")
assert(inspect("Hello") == '"Hello"')

local safefunc = sandbox.protect([[
]])

safefunc()

runf("Script:vec.lua")
METADOT_INFO(i18n("loaded_vec"))

content = i18n("welcome")

local meo = require("meo")
local codes, err, globals = meo.to_lua(
	[[
f = ->
  print "hello meo"
f!
]],
	{
		implicit_return_root = true,
		reserve_line_number = true,
		lint_global = true,
	}
)

f = load(codes)
f()

add_packagepath(METADOT_RESLOC("data/scripts/samples"))
add_packagepath(METADOT_RESLOC("data/assets/ui/imguicss"))

-- runf("Script:tests/test_lpeg.lua")
-- runf("Script:tests/test_ffi.lua")
-- runf("Script:tests/test_lz4.lua")
-- runf("Script:tests/test_csv.lua")
runf("Script:tests/test_p1.lua")
-- runf("Script:tests/test_cs.lua")
runf("Script:tests/test_string.lua")

function starts_with(str, start)
	return str:sub(1, #start) == start
end

function ends_with(str, ending)
	return ending == "" or str:sub(-#ending) == ending
end

corouts = {}

-- will call func in coroutine mode
-- call coroutine.yield(numberOfTicksToWait) 0 = means call next tick
-- coroutine.yield(-1) to stop immediately
function registerCoroutine(func, ticksToWait)
	ticksToWait = ticksToWait or 0
	c = { corou = coroutine.create(func), sleepy_ticks = ticksToWait }
	table.insert(corouts, c)
end

-- can be used only in coroutines
-- will wait for specified number of game ticks
--  0 -> returns immediately
function waitFor(ticks)
	if ticks > 0 then
		coroutine.yield(ticks - 1)
	end
end

-- can be used only in coroutines
-- stops thread and never returns
function exitThread()
	coroutine.yield(-1)
end

function dump(o)
	if type(o) == "table" then
		local s = "{ "
		for k, v in pairs(o) do
			if type(k) ~= "number" then
				k = '"' .. k .. '"'
			end
			s = s .. "[" .. k .. "] = " .. dump(v) .. ","
		end
		return s .. "} "
	else
		return tostring(o)
	end
end
