--[[
Metadot muscript is enhanced based on moonscript and yuescript modification
Metadot code copyright(c) 2022, KaoruXun All rights reserved.
Moonscript code by Leaf Corcoran licensed under the MIT License
Link to https://github.com/leafo/moonscript

Copyright (C) 2020 by Leaf Corcoran

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
]]

local mu = select(1, ...)
local concat, insert = table.concat, table.insert
local unpack = unpack or table.unpack
mu.mu_compiled = {}
mu.file_exist = function(fname)
    local file = io.open(fname)
    if file then
        file:close()
        return true
    else
        return false
    end
end
mu.read_file = function(fname)
    local file, err = io.open(fname)
    if not file then
        return nil, err
    end
    local text = assert(file:read("*a"))
    file:close()
    return text
end
local function get_options(...)
    local count = select("#", ...)
    local opts = select(count, ...)
    if type(opts) == "table" then
        return opts, unpack({
            ...
        }, nil, count - 1)
    else
        return {}, ...
    end
end

local function find_modulepath(name)
    local suffix = "." .. mu.options.extension
    local dirsep = mu.options.dirsep
    local name_path = name:match("[\\/]") and name or name:gsub("%.", dirsep)
    local file_exist, file_path
    local tried = {}
    local paths = { package.path, mu.options.path }
    for i = 1, #paths do
        local mu_path = paths[i]
        for path in mu_path:gmatch("[^;]+") do
            file_path = path:gsub("?", name_path):gsub("%.lua$", suffix)
            file_exist = mu.file_exist(file_path)
            if file_exist then
                break
            else
                tried[#tried + 1] = file_path
            end
        end
    end
    if file_exist then
        return file_path
    else
        return nil, tried
    end
end

local mu_loadstring
local function mu_loader(name)
    local file_path, tried = mu.find_modulepath(name)
    if file_path then
        local text = mu.read_file(file_path)
        if text then
            local res, err = mu_loadstring(text, file_path)
            if not res then
                error(file_path .. ": " .. err)
            end
            return res
        else
            return "no file '" .. file_path .. "'"
        end
    end
    for i = 1, #tried do
        tried[i] = "no file '" .. tried[i] .. "'"
    end
    return concat(tried, "\n\t")
end

local function mu_call(f, ...)
    local args = {
        ...
    }
    return xpcall((function()
        return f(unpack(args))
    end), function(err)
        return mu.traceback(err, 1)
    end)
end

mu_loadstring = function(...)
    local options, str, chunk_name, mode, env = get_options(...)
    chunk_name = chunk_name or "=(muscript.loadstring)"
    options.module = chunk_name
    local code, err = mu.to_lua(str, options)
    if not code then
        return nil, err
    end
    if chunk_name then
        mu.mu_compiled["@" .. chunk_name] = code
    end
    return (loadstring or load)(code, chunk_name, unpack({
        mode,
        env
    }))
end
local function mu_loadfile(fname, ...)
    local text = mu.read_file(fname)
    return mu_loadstring(text, fname, ...)
end

local function mu_dofile(...)
    local f = assert(mu_loadfile(...))
    return f()
end

local function insert_loader(pos)
    if pos == nil then
        pos = 3
    end
    local loaders = package.loaders or package.searchers
    for i = 1, #loaders do
        local loader = loaders[i]
        if loader == mu_loader then
            return false
        end
    end
    insert(loaders, pos, mu_loader)
    return true
end

mu.options.dump_locals = false
mu.options.simplified = true
local load_stacktraceplus = mu.load_stacktraceplus
mu.load_stacktraceplus = nil
local stp
local function mu_traceback(err, level)
    if not stp then
        stp = load_stacktraceplus()
    end
    stp.dump_locals = mu.options.dump_locals
    stp.simplified = mu.options.simplified
    return stp.stacktrace(err, level)
end

local function mu_require(name)
    insert_loader()
    local success, res = xpcall(function()
        return require(name)
    end, function(err)
        return mu_traceback(err, 2)
    end)
    if success then
        return res
    else
        print(res)
        return nil
    end
end

setmetatable(mu, {
    __call = function(self, name)
        return self.require(name)
    end
})
local function dump(what)
    local seen = {}
    local _dump
    _dump = function(what, depth)
        depth = depth or 0
        local t = type(what)
        if "string" == t then
            return "\"" .. tostring(what) .. "\"\n"
        elseif "table" == t then
            if seen[what] then
                return "recursion(" .. tostring(what) .. ")...\n"
            end
            seen[what] = true
            depth = depth + 1
            local lines = {}
            for k, v in pairs(what) do
                insert(lines, ('\t'):rep(depth) .. "[" .. tostring(k) .. "] = " .. _dump(v, depth))
            end
            seen[what] = false
            return "{\n" .. concat(lines) .. ('\t'):rep(depth - 1) .. "}\n"
        else
            return tostring(what) .. "\n"
        end
    end
    return _dump(what)
end

local function p(...)
    local args = { ... }
    for i = 1, #args do
        args[i] = dump(args[i])
    end
    print(concat(args))
end

mu.find_modulepath = find_modulepath
mu.insert_loader = insert_loader
mu.dofile = mu_dofile
mu.loadfile = mu_loadfile
mu.loadstring = mu_loadstring
mu.pcall = mu_call
mu.require = mu_require
mu.p = p
mu.traceback = mu_traceback

METADOT_INFO("MuScript initialized!")
