-- Copyright(c) 2022, KaoruXun All rights reserved.

local lpeg = require "lpeg"
local re = require "re2"
local cosmo = require "cosmo"

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

metadot_lua.define("mu",
    "{.*} -> build_chunk",
    function(c)
        return ""
    end,
    { build_chunk = function(c)
        return print(c)
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
