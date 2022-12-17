local base = {}

local pairs, ipairs = pairs, ipairs
local type, assert, unpack = type, assert, unpack or table.unpack
local tostring, tonumber = tostring, tonumber
local math_floor = math.floor
local math_ceil = math.ceil
local math_atan2 = math.atan2 or math.atan
local math_sqrt = math.sqrt
local math_abs = math.abs

local noop = function()
end

local identity = function(x)
    return x
end

local patternescape = function(str)
    return str:gsub("[%(%)%.%%%+%-%*%?%[%]%^%$]", "%%%1")
end

local absindex = function(len, i)
    return i < 0 and (len + i + 1) or i
end

local iscallable = function(x)
    if type(x) == "function" then return true end
    local mt = getmetatable(x)
    return mt and mt.__call ~= nil
end

local getiter = function(x)
    if base.isarray(x) then
        return ipairs
    elseif type(x) == "table" then
        return pairs
    end
    error("expected table", 3)
end

local iteratee = function(x)
    if x == nil then return identity end
    if iscallable(x) then return x end
    if type(x) == "table" then
        return function(z)
            for k, v in pairs(x) do
                if z[k] ~= v then return false end
            end
            return true
        end
    end
    return function(z) return z[x] end
end



function base.clamp(x, min, max)
    return x < min and min or (x > max and max or x)
end

function base.round(x, increment)
    if increment then return base.round(x / increment) * increment end
    return x >= 0 and math_floor(x + .5) or math_ceil(x - .5)
end

function base.sign(x)
    return x < 0 and -1 or 1
end

function base.lerp(a, b, amount)
    return a + (b - a) * base.clamp(amount, 0, 1)
end

function base.smooth(a, b, amount)
    local t = base.clamp(amount, 0, 1)
    local m = t * t * (3 - 2 * t)
    return a + (b - a) * m
end

function base.pingpong(x)
    return 1 - math_abs(1 - x % 2)
end

function base.distance(x1, y1, x2, y2, squared)
    local dx = x1 - x2
    local dy = y1 - y2
    local s = dx * dx + dy * dy
    return squared and s or math_sqrt(s)
end

function base.angle(x1, y1, x2, y2)
    return math_atan2(y2 - y1, x2 - x1)
end

function base.vector(angle, magnitude)
    return math.cos(angle) * magnitude, math.sin(angle) * magnitude
end

function base.random(a, b)
    if not a then a, b = 0, 1 end
    if not b then b = 0 end
    return a + math.random() * (b - a)
end

function base.randomchoice(t)
    return t[math.random(#t)]
end

function base.weightedchoice(t)
    local sum = 0
    for _, v in pairs(t) do
        assert(v >= 0, "weight value less than zero")
        sum = sum + v
    end
    assert(sum ~= 0, "all weights are zero")
    local rnd = base.random(sum)
    for k, v in pairs(t) do
        if rnd < v then return k end
        rnd = rnd - v
    end
end

function base.isarray(x)
    return type(x) == "table" and x[1] ~= nil
end

function base.push(t, ...)
    local n = select("#", ...)
    for i = 1, n do
        t[#t + 1] = select(i, ...)
    end
    return ...
end

function base.remove(t, x)
    local iter = getiter(t)
    for i, v in iter(t) do
        if v == x then
            if base.isarray(t) then
                table.remove(t, i)
                break
            else
                t[i] = nil
                break
            end
        end
    end
    return x
end

function base.clear(t)
    local iter = getiter(t)
    for k in iter(t) do
        t[k] = nil
    end
    return t
end

function base.extend(t, ...)
    for i = 1, select("#", ...) do
        local x = select(i, ...)
        if x then
            for k, v in pairs(x) do
                t[k] = v
            end
        end
    end
    return t
end

function base.shuffle(t)
    local rtn = {}
    for i = 1, #t do
        local r = math.random(i)
        if r ~= i then
            rtn[i] = rtn[r]
        end
        rtn[r] = t[i]
    end
    return rtn
end

function base.sort(t, comp)
    local rtn = base.clone(t)
    if comp then
        if type(comp) == "string" then
            table.sort(rtn, function(a, b) return a[comp] < b[comp] end)
        else
            table.sort(rtn, comp)
        end
    else
        table.sort(rtn)
    end
    return rtn
end

function base.array(...)
    local t = {}
    for x in ... do t[#t + 1] = x end
    return t
end

function base.each(t, fn, ...)
    local iter = getiter(t)
    if type(fn) == "string" then
        for _, v in iter(t) do v[fn](v, ...) end
    else
        for _, v in iter(t) do fn(v, ...) end
    end
    return t
end

function base.map(t, fn)
    fn = iteratee(fn)
    local iter = getiter(t)
    local rtn = {}
    for k, v in iter(t) do rtn[k] = fn(v) end
    return rtn
end

function base.all(t, fn)
    fn = iteratee(fn)
    local iter = getiter(t)
    for _, v in iter(t) do
        if not fn(v) then return false end
    end
    return true
end

function base.any(t, fn)
    fn = iteratee(fn)
    local iter = getiter(t)
    for _, v in iter(t) do
        if fn(v) then return true end
    end
    return false
end

function base.reduce(t, fn, first)
    local started = first ~= nil
    local acc = first
    local iter = getiter(t)
    for _, v in iter(t) do
        if started then
            acc = fn(acc, v)
        else
            acc = v
            started = true
        end
    end
    assert(started, "reduce of an empty table with no first value")
    return acc
end

function base.unique(t)
    local rtn = {}
    for k in pairs(base.invert(t)) do
        rtn[#rtn + 1] = k
    end
    return rtn
end

function base.filter(t, fn, retainkeys)
    fn = iteratee(fn)
    local iter = getiter(t)
    local rtn = {}
    if retainkeys then
        for k, v in iter(t) do
            if fn(v) then rtn[k] = v end
        end
    else
        for _, v in iter(t) do
            if fn(v) then rtn[#rtn + 1] = v end
        end
    end
    return rtn
end

function base.reject(t, fn, retainkeys)
    fn = iteratee(fn)
    local iter = getiter(t)
    local rtn = {}
    if retainkeys then
        for k, v in iter(t) do
            if not fn(v) then rtn[k] = v end
        end
    else
        for _, v in iter(t) do
            if not fn(v) then rtn[#rtn + 1] = v end
        end
    end
    return rtn
end

function base.merge(...)
    local rtn = {}
    for i = 1, select("#", ...) do
        local t = select(i, ...)
        local iter = getiter(t)
        for k, v in iter(t) do
            rtn[k] = v
        end
    end
    return rtn
end

function base.concat(...)
    local rtn = {}
    for i = 1, select("#", ...) do
        local t = select(i, ...)
        if t ~= nil then
            local iter = getiter(t)
            for _, v in iter(t) do
                rtn[#rtn + 1] = v
            end
        end
    end
    return rtn
end

function base.find(t, value)
    local iter = getiter(t)
    for k, v in iter(t) do
        if v == value then return k end
    end
    return nil
end

function base.match(t, fn)
    fn = iteratee(fn)
    local iter = getiter(t)
    for k, v in iter(t) do
        if fn(v) then return v, k end
    end
    return nil
end

function base.count(t, fn)
    local count = 0
    local iter = getiter(t)
    if fn then
        fn = iteratee(fn)
        for _, v in iter(t) do
            if fn(v) then count = count + 1 end
        end
    else
        if base.isarray(t) then
            return #t
        end
        for _ in iter(t) do count = count + 1 end
    end
    return count
end

function base.slice(t, i, j)
    i = i and absindex(#t, i) or 1
    j = j and absindex(#t, j) or #t
    local rtn = {}
    for x = i < 1 and 1 or i, j > #t and #t or j do
        rtn[#rtn + 1] = t[x]
    end
    return rtn
end

function base.first(t, n)
    if not n then return t[1] end
    return base.slice(t, 1, n)
end

function base.last(t, n)
    if not n then return t[#t] end
    return base.slice(t, -n, -1)
end

function base.invert(t)
    local rtn = {}
    for k, v in pairs(t) do rtn[v] = k end
    return rtn
end

function base.pick(t, ...)
    local rtn = {}
    for i = 1, select("#", ...) do
        local k = select(i, ...)
        rtn[k] = t[k]
    end
    return rtn
end

function base.keys(t)
    local rtn = {}
    local iter = getiter(t)
    for k in iter(t) do rtn[#rtn + 1] = k end
    return rtn
end

function base.clone(t)
    local rtn = {}
    for k, v in pairs(t) do rtn[k] = v end
    return rtn
end

function base.fn(fn, ...)
    assert(iscallable(fn), "expected a function as the first argument")
    local args = { ... }
    return function(...)
        local a = base.concat(args, { ... })
        return fn(unpack(a))
    end
end

function base.once(fn, ...)
    local f = base.fn(fn, ...)
    local done = false
    return function(...)
        if done then return end
        done = true
        return f(...)
    end
end

local memoize_fnkey = {}
local memoize_nil = {}

function base.memoize(fn)
    local cache = {}
    return function(...)
        local c = cache
        for i = 1, select("#", ...) do
            local a = select(i, ...) or memoize_nil
            c[a] = c[a] or {}
            c = c[a]
        end
        c[memoize_fnkey] = c[memoize_fnkey] or { fn(...) }
        return unpack(c[memoize_fnkey])
    end
end

function base.combine(...)
    local n = select('#', ...)
    if n == 0 then return noop end
    if n == 1 then
        local fn = select(1, ...)
        if not fn then return noop end
        assert(iscallable(fn), "expected a function or nil")
        return fn
    end
    local funcs = {}
    for i = 1, n do
        local fn = select(i, ...)
        if fn ~= nil then
            assert(iscallable(fn), "expected a function or nil")
            funcs[#funcs + 1] = fn
        end
    end
    return function(...)
        for _, f in ipairs(funcs) do f(...) end
    end
end

function base.call(fn, ...)
    if fn then
        return fn(...)
    end
end

function base.time(fn, ...)
    local start = os.clock()
    local rtn = { fn(...) }
    return (os.clock() - start), unpack(rtn)
end

local lambda_cache = {}

function base.lambda(str)
    if not lambda_cache[str] then
        local args, body = str:match([[^([%w,_ ]-)%->(.-)$]])
        assert(args and body, "bad string lambda")
        local s = "return function(" .. args .. ")\nreturn " .. body .. "\nend"
        lambda_cache[str] = base.dostring(s)
    end
    return lambda_cache[str]
end

local serialize

local serialize_map = {
    ["boolean"] = tostring,
    ["nil"] = tostring,
    ["string"] = function(v) return string.format("%q", v) end,
    ["number"] = function(v)
        if v ~= v then return "0/0" --  nan
        elseif v == 1 / 0 then return "1/0" --  inf
        elseif v == -1 / 0 then return "-1/0" end -- -inf
        return tostring(v)
    end,
    ["table"] = function(t, stk)
        stk = stk or {}
        if stk[t] then error("circular reference") end
        local rtn = {}
        stk[t] = true
        for k, v in pairs(t) do
            rtn[#rtn + 1] = "[" .. serialize(k, stk) .. "]=" .. serialize(v, stk)
        end
        stk[t] = nil
        return "{" .. table.concat(rtn, ",") .. "}"
    end
}

setmetatable(serialize_map, {
    __index = function(_, k) error("unsupported serialize type: " .. k) end
})

serialize = function(x, stk)
    return serialize_map[type(x)](x, stk)
end

function base.serialize(x)
    return serialize(x)
end

function base.deserialize(str)
    return base.dostring("return " .. str)
end

function base.split(str, sep)
    if not sep then
        return base.array(str:gmatch("([%S]+)"))
    else
        assert(sep ~= "", "empty separator")
        local psep = patternescape(sep)
        return base.array((str .. sep):gmatch("(.-)(" .. psep .. ")"))
    end
end

function base.trim(str, chars)
    if not chars then return str:match("^[%s]*(.-)[%s]*$") end
    chars = patternescape(chars)
    return str:match("^[" .. chars .. "]*(.-)[" .. chars .. "]*$")
end

function base.wordwrap(str, limit)
    limit = limit or 72
    local check
    if type(limit) == "number" then
        check = function(s) return #s >= limit end
    else
        check = limit
    end
    local rtn = {}
    local line = ""
    for word, spaces in str:gmatch("(%S+)(%s*)") do
        local s = line .. word
        if check(s) then
            table.insert(rtn, line .. "\n")
            line = word
        else
            line = s
        end
        for c in spaces:gmatch(".") do
            if c == "\n" then
                table.insert(rtn, line .. "\n")
                line = ""
            else
                line = line .. c
            end
        end
    end
    table.insert(rtn, line)
    return table.concat(rtn)
end

function base.format(str, vars)
    if not vars then return str end
    local f = function(x)
        return tostring(vars[x] or vars[tonumber(x)] or "{" .. x .. "}")
    end
    return (str:gsub("{(.-)}", f))
end

function base.trace(...)
    local info = debug.getinfo(2, "Sl")
    local t = { info.short_src .. ":" .. info.currentline .. ":" }
    for i = 1, select("#", ...) do
        local x = select(i, ...)
        if type(x) == "number" then
            x = string.format("%g", base.round(x, .01))
        end
        t[#t + 1] = tostring(x)
    end
    print(table.concat(t, " "))
end

function base.dostring(str)
    return assert((load)(str))()
end

function base.uuid()
    local fn = function(x)
        local r = math.random(16) - 1
        r = (x == "x") and (r + 1) or (r % 4) + 9
        return ("0123456789abcdef"):sub(r, r)
    end
    return (("xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx"):gsub("[xy]", fn))
end

function base.hotswap(modname)
    local oldglobal = base.clone(_G)
    local updated = {}
    local function update(old, new)
        if updated[old] then return end
        updated[old] = true
        local oldmt, newmt = getmetatable(old), getmetatable(new)
        if oldmt and newmt then update(oldmt, newmt) end
        for k, v in pairs(new) do
            if type(v) == "table" then update(old[k], v) else old[k] = v end
        end
    end

    local err = nil
    local function onerror(e)
        for k in pairs(_G) do _G[k] = oldglobal[k] end
        err = base.trim(e)
    end

    local ok, oldmod = pcall(require, modname)
    oldmod = ok and oldmod or nil
    xpcall(function()
        package.loaded[modname] = nil
        local newmod = require(modname)
        if type(oldmod) == "table" then update(oldmod, newmod) end
        for k, v in pairs(oldglobal) do
            if v ~= _G[k] and type(v) == "table" then
                update(v, _G[k])
                _G[k] = v
            end
        end
    end, onerror)
    package.loaded[modname] = oldmod
    if err then return nil, err end
    return oldmod
end

local ripairs_iter = function(t, i)
    i = i - 1
    local v = t[i]
    if v ~= nil then
        return i, v
    end
end

function base.ripairs(t)
    return ripairs_iter, t, (#t + 1)
end

function base.color(str, mul)
    mul = mul or 1
    local r, g, b, a
    r, g, b = str:match("#(%x%x)(%x%x)(%x%x)")
    if r then
        r = tonumber(r, 16) / 0xff
        g = tonumber(g, 16) / 0xff
        b = tonumber(b, 16) / 0xff
        a = 1
    elseif str:match("rgba?%s*%([%d%s%.,]+%)") then
        local f = str:gmatch("[%d.]+")
        r = (f() or 0) / 0xff
        g = (f() or 0) / 0xff
        b = (f() or 0) / 0xff
        a = f() or 1
    else
        error(("bad color string '%s'"):format(str))
    end
    return r * mul, g * mul, b * mul, a * mul
end

local chain_mt = {}
chain_mt.__index = base.map(base.filter(base, iscallable, true),
    function(fn)
        return function(self, ...)
            self._value = fn(self._value, ...)
            return self
        end
    end)
chain_mt.__index.result = function(x) return x._value end

function base.chain(value)
    return setmetatable({ _value = value }, chain_mt)
end

setmetatable(base, {
    __call = function(_, ...)
        return base.chain(...)
    end
})

local metadot_base = {}
metadot_base.base = base

return metadot_base
