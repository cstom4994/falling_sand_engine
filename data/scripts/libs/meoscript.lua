--[[
Metadot MeoScript is enhanced based on moonscript
Metadot Code Copyright(c) 2022-2023, KaoruXun All rights reserved.

Link to https://github.com/leafo/moonscript

Copyright (C) 2020 by Leaf Corcoran

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
]]

local meo = select(1, ...)
local concat, insert = table.concat, table.insert
local unpack = unpack or table.unpack
meo.meo_compiled = {}
meo.file_exist = function(fname)
    local file = io.open(fname)
    if file then
        file:close()
        return true
    else
        return false
    end
end
meo.read_file = function(fname)
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
    local suffix = "." .. meo.options.extension
    local dirsep = meo.options.dirsep
    local name_path = name:match("[\\/]") and name or name:gsub("%.", dirsep)
    local file_exist, file_path
    local tried = {}
    local paths = { package.path, meo.options.path }
    for i = 1, #paths do
        local meo_path = paths[i]
        for path in meo_path:gmatch("[^;]+") do
            file_path = path:gsub("?", name_path):gsub("%.lua$", suffix)
            file_exist = meo.file_exist(file_path)
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

local meo_loadstring
local function meo_loader(name)
    local file_path, tried = meo.find_modulepath(name)
    if file_path then
        local text = meo.read_file(file_path)
        if text then
            local res, err = meo_loadstring(text, file_path)
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

local function meo_call(f, ...)
    local args = {
        ...
    }
    return xpcall((function()
        return f(unpack(args))
    end), function(err)
        return meo.traceback(err, 1)
    end)
end

meo_loadstring = function(...)
    local options, str, chunk_name, mode, env = get_options(...)
    chunk_name = chunk_name or "=(meoscript.loadstring)"
    options.module = chunk_name
    local code, err = meo.to_lua(str, options)
    if not code then
        return nil, err
    end
    if chunk_name then
        meo.meo_compiled["@" .. chunk_name] = code
    end
    return (loadstring or load)(code, chunk_name, unpack({
        mode,
        env
    }))
end
local function meo_loadfile(fname, ...)
    local text = meo.read_file(fname)
    return meo_loadstring(text, fname, ...)
end

local function meo_dofile(...)
    local f = assert(meo_loadfile(...))
    return f()
end

local function insert_loader(pos)
    if pos == nil then
        pos = 3
    end
    local loaders = package.loaders or package.searchers
    for i = 1, #loaders do
        local loader = loaders[i]
        if loader == meo_loader then
            return false
        end
    end
    insert(loaders, pos, meo_loader)
    return true
end

meo.options.dump_locals = false
meo.options.simplified = true
local load_stacktraceplus = meo.load_stacktraceplus
meo.load_stacktraceplus = nil
local stp
local function meo_traceback(err, level)
    if not stp then
        stp = load_stacktraceplus()
    end
    stp.dump_locals = meo.options.dump_locals
    stp.simplified = meo.options.simplified
    return stp.stacktrace(err, level)
end

local function meo_require(name)
    insert_loader()
    local success, res = xpcall(function()
        return require(name)
    end, function(err)
        return meo_traceback(err, 2)
    end)
    if success then
        return res
    else
        print(res)
        return nil
    end
end

setmetatable(meo, {
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

meo.find_modulepath = find_modulepath
meo.insert_loader = insert_loader
meo.dofile = meo_dofile
meo.loadfile = meo_loadfile
meo.loadstring = meo_loadstring
meo.pcall = meo_call
meo.require = meo_require
meo.p = p
meo.traceback = meo_traceback

METADOT_INFO("MeoScript initialized!")
