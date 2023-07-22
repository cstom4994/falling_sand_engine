-- Copyright(c) 2022-2023, KaoruXun All rights reserved.
-- runs on start of the engine
-- used to load scripts
tl = require("tl")
inspect = require("inspect")
sandbox = require("sandbox")
lang = require("lang")

assert(inspect({1, 2, 3, 4}) == "{ 1, 2, 3, 4 }")
assert(inspect(1) == "1")
assert(inspect("Hello") == '"Hello"')

local safefunc = sandbox.protect([[
]])

safefunc()

autoload("LUA::vec.lua")

METADOT_INFO(i18n("loaded_vec"))

content = i18n("welcome")

-- add_packagepath(METADOT_RESLOC("data/scripts/samples"))

-- autoload("LUA::tests/test_ffi.lua")
-- autoload("LUA::tests/test_lz4.lua")
-- autoload("LUA::tests/test_csv.lua")
-- autoload("LUA::tests/test_p1.lua")
-- autoload("LUA::tests/test_cs.lua")
-- autoload("LUA::tests/test_string.lua")

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
    c = {
        corou = coroutine.create(func),
        sleepy_ticks = ticksToWait
    }
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
