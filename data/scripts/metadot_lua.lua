-- Copyright(c) 2022-2023, KaoruXun All rights reserved.

local lpeg = require("lpeg")
local re = require("re2")
local cosmo = require("cosmo")

-- begin js parser

function letOperator(code)
    code = code:gsub("let %s*(%w+)%s*=", "local %1 =")
    code = code:gsub("var %s*(%w+)%s*=", "%1 =")

    return code
end

function constOperator(code)
    -- find all consts
    local consts = {}

    for const in code:gmatch("const %s*(%w+)") do
        table.insert(consts, const)
    end

    for k, v in pairs(consts) do
        code = code:gsub("const %s*" .. v .. "%s*= ?%d?%s*", function(match)
            local f = match:find("=")
            return "local " .. v .. " = --[[;__DO NOT CAPTURE__]] " .. match:sub(f + 1)
        end)

        -- make assert(false, "can't change const value") if const is changed, don't capture from above
        code = code:gsub(v .. "%s*= %s*(%w+)", "assert(false, \"Tried to change const value for " .. v .. "\")")
    end

    return code
end

function onlynumbers(str)
    local c = ""
    for i = 1, #str do
        if str:sub(i, i):match("%d") then
            c = c .. str:sub(i, i)
        end
    end
    return tonumber(c)
end

function quickOperators(code)
    -- replace all variable++ with variable = variable + 1
    code = code:gsub("(%w+)%+%+", function(variable)
        return variable .. " = " .. variable .. " + 1"
    end)

    -- same with --
    code = code:gsub("(%w+)%-%-", function(variable)
        return variable .. " = " .. variable .. " - 1"
    end)

    -- replace all variable += count with variable = variable + count
    code = code:gsub("(%w+) ?%+= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " + " .. count
    end)

    code = code:gsub("(%w+) ?%-%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " - " .. count
    end)

    code = code:gsub("(%w+) ?%*%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " * " .. count
    end)

    code = code:gsub("(%w+) ?%/%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " / " .. count
    end)

    code = code:gsub("(%w+) ?%%%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " % " .. count
    end)

    code = code:gsub("(%w+) ?%^%= ?(%d+)", function(variable, count)
        return variable .. " = " .. variable .. " ^ " .. count
    end)

    return code
end

findall = function(code, find)
    local found = {}
    local lastPos = 0
    while code:find(find, lastPos) do
        local pos = code:find(find, lastPos)
        table.insert(found, pos)
        lastPos = pos + find:len()
    end
    return found
end

function arrowFunctions(code)
    -- replace if(condition) { body } with if(condition) then body end
    code = code:gsub("if(%b()) ?{ ?(.-) ?}", "if%1 then %2 end")

    -- replace argument => { body } with function(argument) body end
    code = code:gsub("(%w+) => ?{ ?(.-) ?}", "function(%1) %2 end")

    -- replace while(condition) { body } with while(condition) do body end
    code = code:gsub("while(%b()) ?{ ?(.-) ?}", "while%1 do %2 end")

    -- replace for(var = start; var < end; var++) { body } with for var = start, end, 1 do body end
    code = code:gsub("for(%b()) ? { ?(.-) ?}", function(a, b)
        local pos = a:find(";")
        if not pos then return end
        local var = a:sub(1, pos - 1)
        var = var:match("%w+")
        if not var then return "assert(false, 'wrong for loop')" end
        if not pos then return "assert(false, 'wrong for loop')" end
        local start = a:sub(1, pos - 1):gsub("%(?%s*(%w+)%s*=%s*", "")
        start = onlynumbers(start)
        if not start then return "assert(false, 'wrong for loop')" end
        local stop = a:sub(pos + 1):gsub("%s*(%w+)%s*<%s*(%w+)%s*;%s*(%w+)%s*%+%+", "%2"):gsub("%)", "")
        local pos = stop:find(";")
        if pos then
            stop = stop:sub(1, pos - 1):gsub("%s*(%w+)%s*%+%+", "")
        end
        stop = onlynumbers(stop)
        if not stop then return "assert(false, 'wrong for loop')" end
        local pos = a:find(";")
        local step = a:sub((pos or 1) + 1):gsub("%s*(%w+)%s*<%s*(%w+)%s*;%s*(%w+)%s*", "%3"):gsub("%)", "")

        -- if step is var++, then it's 1
        if tostring(step):find("%+%+") then
            step = 1
        end

        if tostring(step):find("%-%-") then
            step = -1
        end

        if tostring(step):match("%w* ?= ?%w* ?+ ?%d+") then
            step = step:gsub("%w* ?= ?%w* ?+ ?", "")
        end

        if tostring(step):match("%w* ?= ?%w* ?- ?%d+") then
            step = -step:gsub("%w* ?= ?%w* ?- ?", "")
        end

        if tostring(step):match("%w* ?+= ?%d+") then
            step = step:gsub("%w* ?+= ?", "")
        end

        if tostring(step):match("%w* ?-= ?%d+") then
            step = -step:gsub("%w* ?-= ?", "")
        end

        step = tonumber(step)
        if not step then return "assert(false, 'wrong for loop')" end
        return "for " .. var .. " = " .. start .. ", " .. stop .. ", " .. step .. " do " .. b .. " end"
    end)

    -- replace for(var in array) { body } with for var in pairs(array) do body end
    code = code:gsub("for%s*%((%w+)%s+of%s+(%w+)%) ?{ ?(.-) ?}", function(variable, array, body)
        return "for _," .. variable .. " in pairs(" .. array .. ") do " .. body .. " end"
    end)

    -- replace all for(variable in array) with for variable in ipairs(array)
    code = code:gsub("for%s*%((%w+)%s+in%s+(%w+)%) ?{ ?(.-) ?}", function(variable, array, body)
        return "for " .. variable .. " in pairs(" .. array .. ") do " .. body .. " end"
    end)

    -- replace all for(key, value in array) with for key, value in pairs(array)
    code = code:gsub("for%s*%((%w+)%s*,%s*(%w+)%s*of%s*(%w+)%) ?{ ?(.-) ?}", function(key, value, array, body)
        return "for " .. key .. ", " .. value .. " in pairs(" .. array .. ") do " .. body .. " end"
    end)
    code = code:gsub("for%s*%((%w+)%s*,%s*(%w+)%s*in%s*(%w+)%) ?{ ?(.-) ?}", function(key, value, array, body)
        return "for " .. key .. ", " .. value .. " in pairs(" .. array .. ") do " .. body .. " end"
    end)

    -- replace function() { body } with function() body end
    code = code:gsub("function(%b()) ?{ ?(.-) ?}", "function%1 %2 end")

    -- replace (arguments) => { body } with function(arguments) body end
    code = code:gsub("(%b()) => ?{ ?(.-) ?}", "function%1 %2 end")

    -- replace (arguments) => result; with function(arguments) return result end
    code = code:gsub("(%b()) => ?(.-) ?;", "function%1 return %2 end")

    -- replace all [] with {}
    code = code:gsub(" %[", " {")
    code = code:gsub(" %]", " }")
    code = code:gsub("{%]", "{}")

    -- find all ] with findall, if there is a { before, then replace with }
    local found = findall(code, "%]")
    for i = 1, #found do
        local pos = found[i]
        local before = code:sub(1, pos - 1)
        local after = code:sub(pos + 1)

        local all = findall(before, "%{")
        if #all > 1 then
            local last = all[#all]
            local all = findall(after, "%[")
            if #all > 1 then
                local last2 = all[#all]
                if last2 > last then
                    code = code:sub(1, pos - 1) .. "}" .. code:sub(pos + 1)
                end
            else
                code = code:sub(1, pos - 1) .. "}" .. code:sub(pos + 1)
            end
        end
        -- code = before .. "}" .. after
    end

    -- replace all [number or string} with [number or string]
    code = code:gsub("%[(%w+)%}", "[%1]")
    -- code = code:gsub("%[(%d+)%]", function(a)
    --     return "[" .. a+1 .. "]"
    -- end)

    -- code = code:gsub("for%(%w* ?= ? %d+; ?%w* ?< ?%d+; ?%w*++%) ?{ ?(.-) ?}", function(match)
    --     print(match)
    -- end)

    return code
end

function tryFunctions(code)
    -- replace all try { body } catch { body } with pcall(function() body end)
    code = code:gsub("try ?{ ?(.-) ?} ?catch ?{ ?(.-) ?}", "if not pcall(function() %1 end) then %2 end")

    -- replace all try { body } catch(error) { body } with pcall(function() body end)
    code = code:gsub("try ?{ ?(.-) ?} ?catch%((.-)%) ?{ ?(.-) ?}",
        "local __pres = pcall(function() %1 end); if not __pres then (function(%2) %3 end)(__pres) end")

    return code
end

function jsparser(code)
    code = letOperator(code)
    code = constOperator(code)
    code = arrowFunctions(code)
    code = quickOperators(code)
    code = tryFunctions(code)
    return code
end

-- end js parser

local metadot_lua = {}

local macros = {}

local IGNORED, STRING, LONGSTRING, SHORTSTRING, NAME, NUMBER, BALANCED

do
    local ok, parser = pcall(require, "leg.parser")
    if ok then
        parser.rules.Args = re.compile("balanced <- '[[' ((&'[[' balanced) / (!']]' .))* ']]'") +
            parser.rules.Args
    end
end

do
    local m = lpeg
    local N = m.R '09'
    local AZ = m.R('__', 'az', 'AZ', '\127\255') -- llex.c uses isalpha()

    NAME = AZ * (AZ + N) ^ 0

    local number = (m.P '.' + N) ^ 1 * (m.S 'eE' * m.S '+-' ^ -1) ^ -1 * (N + AZ) ^ 0
    NUMBER = #(N + (m.P '.' * N)) * number

    local long_brackets = #(m.P '[' * m.P '=' ^ 0 * m.P '[') * function(subject, i1)
        local level = _G.assert(subject:match('^%[(=*)%[', i1))
        local _, i2 = subject:find(']' .. level .. ']', i1, true)
        return (i2 and (i2 + 1))
    end

    local multi  = m.P '--' * long_brackets
    local single = m.P '--' * (1 - m.P '\n') ^ 0

    local COMMENT = multi + single
    local SPACE = m.S '\n \t\r\f'
    IGNORED = (SPACE + COMMENT) ^ 0

    SHORTSTRING = (m.P '"' * ((m.P '\\' * 1) + (1 - (m.S '"\n\r\f'))) ^ 0 * m.P '"') +
        (m.P "'" * ((m.P '\\' * 1) + (1 - (m.S "'\n\r\f"))) ^ 0 * m.P "'")
    LONGSTRING = long_brackets
    STRING = SHORTSTRING + LONGSTRING

    BALANCED = re.compile [[
    balanced <- balanced_par / balanced_bra
    balanced_par <- '(' ([^(){}] / balanced)* ')'
    balanced_bra <- '{' ([^(){}] / balanced)* '}'
  ]]
end

local basic_rules = {
    ["_"] = IGNORED,
    name = NAME,
    number = NUMBER,
    string = STRING,
    longstring = LONGSTRING,
    shortstring = SHORTSTRING
}

local function gsub(s, patt, repl)
    patt = lpeg.P(patt)
    patt = lpeg.Cs((patt / repl + 1) ^ 0)
    return lpeg.match(patt, s)
end

metadot_lua.define = function(name, grammar, code, defs)
    defs = defs or {}
    re_defs = {}
    setmetatable(re_defs, { __index = function(t, k)
        local v = defs[k]
        if not v then v = basic_rules[k] end
        rawset(t, k, v)
        return v
    end })
    local patt = re.compile(grammar, re_defs) * (-1)
    if type(code) == "string" then
        code = cosmo.compile(code)
    end
    macros[name] = { patt = patt, code = code }
end

lstring = load

metadot_lua.expand = function(text, filename)
    local macro_use = [=[
    macro <- {name} _ {balanced}
    balanced <- '[[' ((&'[[' balanced) / (!']]' .))* ']]'
  ]=]
    local patt = re.compile(macro_use, basic_rules)
    return gsub(text, patt, function(name, arg)
        if macros[name] then
            arg = string.sub(arg, 3, #arg - 2)
            local patt, code = macros[name].patt, macros[name].code
            local data, err = patt:match(arg)
            if data then
                return metadot_lua.expand((cosmo.fill(tostring(code(data)), data)), filename)
            else
                filename = filename or ("string: " .. text)
                error("parse error on macro " .. name .. ", file " .. filename)
            end
        end
    end)
end

metadot_lua.loadstring = function(text)
    return lstring(metadot_lua.expand(text))
end

metadot_lua.dostring = function(text)
    local ok, f, err = pcall(loadstring, text)
    if ok and f then return f() else error(f or err, 2) end
end

metadot_lua.loadfile = function(filename)
    local file = io.open(filename)
    if file then
        local contents = metadot_lua.expand(string.gsub(file:read("*a"), "^#![^\n]*", ""), filename)
        file:close()
        return lstring(contents, filename)
    else
        error("file " .. filename .. " not found", 2)
    end
end

metadot_lua.dofile = function(filename)
    local ok, f, err = pcall(loadfile, filename)
    if ok and f then return f() else error(f or err, 2) end
end

local function findfile(name)
    local path = package.path
    name = string.gsub(name, "%.", "/")
    for template in string.gmatch(path, "[^;]+") do
        local filename = string.gsub(template, "?", name)
        local file = io.open(filename)
        if file then return file, filename end
    end
end

metadot_lua.loader = function(name)
    local file, filename = findfile(name)
    local ok, contents
    if file then
        ok, contents = pcall(file.read, file, "*a")
        file:close()
        if not ok then return contents end
        ok, contents = pcall(expand, string.gsub(contents, "^#![^\n]*", ""), filename)
        if not ok then return contents end
        return lstring(contents, filename)
    end
end

metadot_lua.define_simple = function(name, code)
    local ok, parser = pcall(require, "leg.parser")
    local exp
    if ok then
        exp = lpeg.P(parser.apply(lpeg.V "Exp"))
    else
        exp = BALANCED + STRING + NAME + NUMBER
    end
    local syntax = [[
    explist <- _ ({exp} _ (',' _ {exp} _)*) -> build_explist
  ]]
    local defs = {
        build_explist = function(...)
            local args = { ... }
            local exps = { args = {} }
            for i, a in ipairs(args) do
                exps[tostring(i)] = a
                exps[i] = a
                exps.args[i] = { value = a }
            end
            return exps
        end,
        exp = exp
    }
    metadot_lua.define(name, syntax, code, defs)
end

metadot_lua.define("require_for_syntax",
    "_ {name} _ (',' _ {name} _)*",
    function(...)
        for _, m in ipairs({ ... }) do
            require(m)
        end
        return ""
    end)

metadot_lua.define("meo",
    "{.*} -> build_chunk",
    function(c)
        return ""
    end,
    { build_chunk = function(c)
        local meo_core = require("meo")
        local codes, err, globals = meo_core.to_lua(c,
            {
                implicit_return_root = true,
                reserve_line_number = true,
                lint_global = true,
            }
        )
        local v = load(codes)
        assert(v, "Error")
        return v()
    end })

metadot_lua.define("meta_js",
    "{.*} -> build_chunk",
    function(c)
        return ""
    end,
    { build_chunk = function(c)
        local v = load(jsparser(c))
        assert(v, "Error")
        return v()
    end })

metadot_lua.define("meta",
    "{.*} -> build_chunk",
    function(c)
        return c() or ""
    end,
    { build_chunk = function(c)
        return metadot_lua.loadstring(c)
    end })

local current_gensym = 0

metadot_lua.gensym = function()
    current_gensym = current_gensym + 1
    return "___metadot_lua_sym_" .. current_gensym
end

return metadot_lua
