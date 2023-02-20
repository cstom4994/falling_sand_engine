// Metadot Code Copyright(c) 2022-2023, KaoruXun All rights reserved.

// https://github.com/pigpigyyy/Yuescript
// https://github.com/axilmar/parserlib

#include "meo_compiler.h"

using namespace std::string_literals;

extern "C" {

#include "libs/lua/host/lauxlib.h"
#include "libs/lua/host/lua.h"
#include "libs/lua/host/lualib.h"

#if LUA_VERSION_NUM > 501
#ifndef LUA_COMPAT_5_1
#ifndef lua_objlen
#define lua_objlen lua_rawlen
#endif  // lua_objlen
#endif  // LUA_COMPAT_5_1
#endif  // LUA_VERSION_NUM

static const char meoscriptCodes[] = R"meoscript_codes(

local meo = select(1, ...)
local concat, insert = table.concat, table.insert
local unpack = unpack or table.unpack
meo.meo_compiled = { }
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
		return { }, ...
	end
end
local function find_modulepath(name)
	local suffix = "." .. meo.options.extension
	local dirsep = meo.options.dirsep
	local name_path = name:match("[\\/]") and name or name:gsub("%.", dirsep)
	local file_exist, file_path
	local tried = {}
	local paths = {package.path, meo.options.path}
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
	local seen = { }
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
	local args = {...}
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
)meoscript_codes";

static void init_meoscript(lua_State* L) {
    if (luaL_loadbuffer(L, meoscriptCodes, sizeof(meoscriptCodes) / sizeof(meoscriptCodes[0]) - 1, "=(meoscript)") != 0) {
        std::string err = "failed to load meoscript module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    } else {
        lua_insert(L, -2);
        if (lua_pcall(L, 1, 0, 0) != 0) {
            std::string err = "failed to init meoscript module.\n"s + lua_tostring(L, -1);
            luaL_error(L, err.c_str());
        }
    }
}

static const char stpCodes[] = R"lua_codes(
--[[
Copyright (c) 2010 Ignacio Burgueño, modified by Li Jin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.]]

-- tables
local _G = _G
local string, io, debug, coroutine = string, io, debug, coroutine

-- functions
local tostring, require = tostring, require
local next, assert = next, assert
local pcall, type, pairs, ipairs = pcall, type, pairs, ipairs
local error = error

assert(debug, "debug table must be available at this point")

local string_gmatch = string.gmatch
local string_sub = string.sub
local table_concat = table.concat

local meo = require("meo")

local _M = {
	max_tb_output_len = 70,	-- controls the maximum length of the 'stringified' table before cutting with ' (more...)'
	dump_locals = true,
	simplified = false
}

-- this tables should be weak so the elements in them won't become uncollectable
local m_known_tables = { [_G] = "_G (global table)" }
local function add_known_module(name, desc)
	local ok, mod = pcall(require, name)
	if ok then
		m_known_tables[mod] = desc
	end
end

add_known_module("string", "string module")
add_known_module("io", "io module")
add_known_module("os", "os module")
add_known_module("table", "table module")
add_known_module("math", "math module")
add_known_module("package", "package module")
add_known_module("debug", "debug module")
add_known_module("coroutine", "coroutine module")

-- lua5.2
add_known_module("bit32", "bit32 module")
-- luajit
add_known_module("bit", "bit module")
add_known_module("jit", "jit module")
-- lua5.3
if _VERSION >= "Lua 5.3" then
	add_known_module("utf8", "utf8 module")
end


local m_user_known_tables = {}

local m_known_functions = {}
for _, name in ipairs{
	-- Lua 5.2, 5.1
	"assert",
	"collectgarbage",
	"dofile",
	"error",
	"getmetatable",
	"ipairs",
	"load",
	"loadfile",
	"next",
	"pairs",
	"pcall",
	"print",
	"rawequal",
	"rawget",
	"rawlen",
	"rawset",
	"require",
	"select",
	"setmetatable",
	"tonumber",
	"tostring",
	"type",
	"xpcall",

	-- Lua 5.1
	"gcinfo",
	"getfenv",
	"loadstring",
	"module",
	"newproxy",
	"setfenv",
	"unpack",
	-- TODO: add table.* etc functions
} do
	if _G[name] then
		m_known_functions[_G[name]] = name
	end
end

local m_user_known_functions = {}

local function safe_tostring (value)
	local ok, err = pcall(tostring, value)
	if ok then return err else return ("<failed to get printable value>: '%s'"):format(err) end
end

-- Private:
-- Parses a line, looking for possible function definitions (in a very naive way)
-- Returns '(anonymous)' if no function name was found in the line
local function ParseLine(line)
	assert(type(line) == "string")
	local match = line:match("^%s*function%s+(%w+)")
	if match then
		--print("+++++++++++++function", match)
		return match
	end
	match = line:match("^%s*local%s+function%s+(%w+)")
	if match then
		--print("++++++++++++local", match)
		return match
	end
	match = line:match("^%s*local%s+(%w+)%s+=%s+function")
	if match then
		--print("++++++++++++local func", match)
		return match
	end
	match = line:match("%s*function%s*%(")	-- this is an anonymous function
	if match then
		--print("+++++++++++++function2", match)
		return "(anonymous)"
	end
	return "(anonymous)"
end

-- Private:
-- Tries to guess a function's name when the debug info structure does not have it.
-- It parses either the file or the string where the function is defined.
-- Returns '?' if the line where the function is defined is not found
local function GuessFunctionName(info)
	-- print("guessing function name")
	if type(info.source) == "string" and info.source:sub(1,1) == "@" then
		local fname = info.source:sub(2)
		local text
		if meo.file_exist(fname) then
			text = meo.read_file(fname)
		end
		if not text then
			-- print("file not found: "..tostring(err))	-- whoops!
			return "?"
		end
		local line
		local count = 0
		for lineText in (text.."\n"):gmatch("(.-)\n") do
			line = lineText
			count = count + 1
			if count == info.linedefined then
				break
			end
		end
		if not line then
			--print("line not found")	-- whoops!
			return "?"
		end
		return ParseLine(line)
	else
		local line
		local lineNumber = 0
		for l in string_gmatch(info.source, "([^\n]+)\n-") do
			lineNumber = lineNumber + 1
			if lineNumber == info.linedefined then
				line = l
				break
			end
		end
		if not line then
			-- print("line not found")	-- whoops!
			return "?"
		end
		return ParseLine(line)
	end
end

---
-- Dumper instances are used to analyze stacks and collect its information.
--
local Dumper = {}

Dumper.new = function(thread)
	local t = { lines = {} }
	for k,v in pairs(Dumper) do t[k] = v end

	t.dumping_same_thread = (thread == coroutine.running())

	-- if a thread was supplied, bind it to debug.info and debug.get
	-- we also need to skip this additional level we are introducing in the callstack (only if we are running
	-- in the same thread we're inspecting)
	if type(thread) == "thread" then
		t.getinfo = function(level, what)
			if t.dumping_same_thread and type(level) == "number" then
				level = level + 1
			end
			return debug.getinfo(thread, level, what)
		end
		t.getlocal = function(level, loc)
			if t.dumping_same_thread then
				level = level + 1
			end
			return debug.getlocal(thread, level, loc)
		end
	else
		t.getinfo = debug.getinfo
		t.getlocal = debug.getlocal
	end

	return t
end

-- helpers for collecting strings to be used when assembling the final trace
function Dumper:add (text)
	self.lines[#self.lines + 1] = text
end
function Dumper:add_f (fmt, ...)
	self:add(fmt:format(...))
end
function Dumper:concat_lines ()
	return table_concat(self.lines)
end

---
-- Private:
-- Iterates over the local variables of a given function.
--
-- @param level The stack level where the function is.
--
function Dumper:DumpLocals (level)
	if not _M.dump_locals then return end

	local prefix = "\t "
	local i = 1

	if self.dumping_same_thread then
		level = level + 1
	end

	local name, value = self.getlocal(level, i)
	if not name then
		return
	end
	self:add("\tLocal variables:\r\n")
	while name do
		if type(value) == "number" then
			self:add_f("%s%s = number: %g\r\n", prefix, name, value)
		elseif type(value) == "boolean" then
			self:add_f("%s%s = boolean: %s\r\n", prefix, name, tostring(value))
		elseif type(value) == "string" then
			self:add_f("%s%s = string: %q\r\n", prefix, name, value)
		elseif type(value) == "userdata" then
			self:add_f("%s%s = %s\r\n", prefix, name, safe_tostring(value))
		elseif type(value) == "nil" then
			self:add_f("%s%s = nil\r\n", prefix, name)
		elseif type(value) == "table" then
			if m_known_tables[value] then
				self:add_f("%s%s = %s\r\n", prefix, name, m_known_tables[value])
			elseif m_user_known_tables[value] then
				self:add_f("%s%s = %s\r\n", prefix, name, m_user_known_tables[value])
			else
				local txt = "{"
				for k,v in pairs(value) do
					txt = txt..safe_tostring(k)..":"..safe_tostring(v)
					if #txt > _M.max_tb_output_len then
						txt = txt.." (more...)"
						break
					end
					if next(value, k) then txt = txt..", " end
				end
				self:add_f("%s%s = %s  %s\r\n", prefix, name, safe_tostring(value), txt.."}")
			end
		elseif type(value) == "function" then
			local info = self.getinfo(value, "nS")
			local fun_name = info.name or m_known_functions[value] or m_user_known_functions[value]
			if info.what == "C" then
				self:add_f("%s%s = C %s\r\n", prefix, name, (fun_name and ("function: " .. fun_name) or tostring(value)))
			else
				local source = info.short_src
				if source:sub(2,7) == "string" then
					source = source:sub(9)
				end
				--for k,v in pairs(info) do print(k,v) end
				fun_name = fun_name or GuessFunctionName(info)
				self:add_f("%s%s = Lua function '%s' (defined at line %d of chunk %s)\r\n", prefix, name, fun_name, info.linedefined, source)
			end
		elseif type(value) == "thread" then
			self:add_f("%sthread %q = %s\r\n", prefix, name, tostring(value))
		end
		i = i + 1
		name, value = self.getlocal(level, i)
	end
end

local function getMeoLineNumber(fname, line)
	local meoCompiled = require("meo").meo_compiled
	local source = meoCompiled["@"..fname]
	if not source then
		source = meoCompiled["@="..fname]
	end
	if not source then
		local name_path = fname:gsub("%.", meo.options.dirsep)
		local file_exist, file_path
		for path in package.path:gmatch("[^;]+") do
			file_path = path:gsub("?", name_path)
			file_exist = meo.file_exist(file_path)
			if file_exist then
				break
			end
		end
		if file_exist then
			local codes = meo.read_file(file_path)
			local meoFile = codes:match("^%s*--%s*%[.*%]:%s*([^\n]*)")
			if meoFile then
				fname = meoFile:gsub("^%s*(.-)%s*$", "%1")
				source = codes
			end
		end
	end
	if source then
		local current, target = 1, tonumber(line)
		local findLine = line
		for lineCode in source:gmatch("([^\n]*)\n") do
			local num = lineCode:match("--%s*(%d+)%s*$")
			if num then
				findLine = num
			end
			if current == target then
				return fname, findLine or line
			end
			current = current + 1
		end
	end
	return fname, line
end

---
-- Public:
-- Collects a detailed stack trace, dumping locals, resolving function names when they're not available, etc.
-- This function is suitable to be used as an error handler with pcall or xpcall
--
-- @param thread An optional thread whose stack is to be inspected (defaul is the current thread)
-- @param message An optional error string or object.
-- @param level An optional number telling at which level to start the traceback (default is 1)
--
-- Returns a string with the stack trace.
--
function _M.stacktrace(thread, message, level)
	if type(thread) ~= "thread" then
		-- shift parameters left
		thread, message, level = nil, thread, message
	end

	thread = thread or coroutine.running()

	level = level or 1

	local dumper = Dumper.new(thread)

	if type(message) == "table" then
		dumper:add("an error object {\r\n")
		local first = true
		for k,v in pairs(message) do
			if first then
				dumper:add("  ")
				first = false
			else
				dumper:add(",\r\n  ")
			end
			dumper:add(safe_tostring(k))
			dumper:add(": ")
			dumper:add(safe_tostring(v))
		end
		dumper:add("\r\n}")
	elseif type(message) == "string" then
		local fname, line, msg = message:match('([^\n]+):(%d+): (.*)$')
		if fname then
			local nfname, nmsg = fname:match('(.+):%d+: (.*)$')
			if nfname then
				fname = nmsg
			end
		end
		if fname then
			local fn = fname:match("%[string \"(.-)\"%]")
			if fn then fname = fn end
			fname = fname:gsub("^%s*(.-)%s*$", "%1")
			fname, line = getMeoLineNumber(fname, line)
			if _M.simplified then
				message = table.concat({
					"", fname, ":",
					line, ": ", msg})
				message = message:gsub("^%(meoscript%):%s*%d+:%s*", "")
				message = message:gsub("%s(%d+):", "%1:")
			else
				message = table.concat({
					"[string \"", fname, "\"]:",
					line, ": ", msg})
			end
		end
		dumper:add(message)
	end

	dumper:add("\r\n")
	dumper:add[[
Stack Traceback
===============
]]

	local level_to_show = 1
	if dumper.dumping_same_thread then level = level + 1 end

	local info = dumper.getinfo(level, "nSlf")
	while info do
		if info.source and info.source:sub(1,1) == "@" then
			info.source = info.source:sub(2)
		elseif info.what == "main" or info.what == "Lua" then
			info.source = info.source
		end
		info.source, info.currentline = getMeoLineNumber(info.source, info.currentline)
		if info.what == "main" then
			if _M.simplified then
				dumper:add_f("(%d) '%s':%d\r\n", level_to_show, info.source, info.currentline)
			else
				dumper:add_f("(%d) main chunk of file '%s' at line %d\r\n", level_to_show, info.source, info.currentline)
			end
		elseif info.what == "C" then
			--print(info.namewhat, info.name)
			--for k,v in pairs(info) do print(k,v, type(v)) end
			local function_name = m_user_known_functions[info.func] or m_known_functions[info.func] or info.name or tostring(info.func)
			dumper:add_f("(%d) %s C function '%s'\r\n", level_to_show, info.namewhat, function_name)
			--dumper:add_f("%s%s = C %s\r\n", prefix, name, (m_known_functions[value] and ("function: " .. m_known_functions[value]) or tostring(value)))
		elseif info.what == "tail" then
			--print("tail")
			--for k,v in pairs(info) do print(k,v, type(v)) end--print(info.namewhat, info.name)
			dumper:add_f("(%d) tail call\r\n", level_to_show)
			dumper:DumpLocals(level)
		elseif info.what == "Lua" then
			local source = info.source
			local function_name = m_user_known_functions[info.func] or m_known_functions[info.func] or info.name
			if source:sub(2, 7) == "string" then
				source = source:sub(10,-3)
			end
			local was_guessed = false
			if not function_name or function_name == "?" then
				--for k,v in pairs(info) do print(k,v, type(v)) end
				function_name = GuessFunctionName(info)
				was_guessed = true
			end
			-- test if we have a file name
			local function_type = (info.namewhat == "") and "function" or info.namewhat
			if info.source and info.source:sub(1, 1) == "@" then
				if _M.simplified then
					dumper:add_f("(%d) '%s':%d%s\r\n", level_to_show, info.source:sub(2), info.currentline, was_guessed and " (guess)" or "")
				else
					dumper:add_f("(%d) Lua %s '%s' at file '%s':%d%s\r\n", level_to_show, function_type, function_name, info.source:sub(2), info.currentline, was_guessed and " (best guess)" or "")
				end
			elseif info.source and info.source:sub(1,1) == '#' then
				if _M.simplified then
					dumper:add_f("(%d) '%s':%d%s\r\n", level_to_show, info.source:sub(2), info.currentline, was_guessed and " (guess)" or "")
				else
					dumper:add_f("(%d) Lua %s '%s' at template '%s':%d%s\r\n", level_to_show, function_type, function_name, info.source:sub(2), info.currentline, was_guessed and " (best guess)" or "")
				end
			else
				if _M.simplified then
					dumper:add_f("(%d) '%s':%d\r\n", level_to_show, source, info.currentline)
				else
					dumper:add_f("(%d) Lua %s '%s' at chunk '%s':%d\r\n", level_to_show, function_type, function_name, source, info.currentline)
				end
			end
			dumper:DumpLocals(level)
		else
			dumper:add_f("(%d) unknown frame %s\r\n", level_to_show, info.what)
		end

		level = level + 1
		level_to_show = level_to_show + 1
		info = dumper.getinfo(level, "nSlf")
	end

	return dumper:concat_lines()
end

--
-- Adds a table to the list of known tables
function _M.add_known_table(tab, description)
	if m_known_tables[tab] then
		error("Cannot override an already known table")
	end
	m_user_known_tables[tab] = description
end

--
-- Adds a function to the list of known functions
function _M.add_known_function(fun, description)
	if m_known_functions[fun] then
		error("Cannot override an already known function")
	end
	m_user_known_functions[fun] = description
end

return _M

)lua_codes";

static int init_stacktraceplus(lua_State* L) {
    if (luaL_loadbuffer(L, stpCodes, sizeof(stpCodes) / sizeof(stpCodes[0]) - 1, "=(stacktraceplus)") != 0) {
        std::string err = "failed to load stacktraceplus module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    } else if (lua_pcall(L, 0, 1, 0) != 0) {
        std::string err = "failed to init stacktraceplus module.\n"s + lua_tostring(L, -1);
        luaL_error(L, err.c_str());
    }
    return 1;
}

static int meotolua(lua_State* L) {
    size_t size = 0;
    const char* input = luaL_checklstring(L, 1, &size);
    meo::MeoConfig config;
    bool sameModule = false;
    if (lua_gettop(L) == 2) {
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_pushliteral(L, "lint_global");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            config.lintGlobalVariable = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "implicit_return_root");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            config.implicitReturnRoot = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "reserve_line_number");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            config.reserveLineNumber = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "space_over_tab");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            config.useSpaceOverTab = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "same_module");
        lua_gettable(L, -2);
        if (lua_isboolean(L, -1) != 0) {
            sameModule = lua_toboolean(L, -1) != 0;
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "line_offset");
        lua_gettable(L, -2);
        if (lua_isnumber(L, -1) != 0) {
            config.lineOffset = static_cast<int>(lua_tonumber(L, -1));
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "module");
        lua_gettable(L, -2);
        if (lua_isstring(L, -1) != 0) {
            config.module = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
        lua_pushliteral(L, "target");
        lua_gettable(L, -2);
        if (lua_isstring(L, -1) != 0) {
            config.options["target"] = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }
    std::string s(input, size);
    auto result = meo::MeoCompiler(L, nullptr, sameModule).compile(s, config);
    if (result.codes.empty() && !result.error.empty()) {
        lua_pushnil(L);
    } else {
        lua_pushlstring(L, result.codes.c_str(), result.codes.size());
    }
    if (result.error.empty()) {
        lua_pushnil(L);
    } else {
        lua_pushlstring(L, result.error.c_str(), result.error.size());
    }
    if (result.globals) {
        lua_createtable(L, static_cast<int>(result.globals->size()), 0);
        int i = 1;
        for (const auto& var : *result.globals) {
            lua_createtable(L, 3, 0);
            lua_pushlstring(L, var.name.c_str(), var.name.size());
            lua_rawseti(L, -2, 1);
            lua_pushinteger(L, var.line);
            lua_rawseti(L, -2, 2);
            lua_pushinteger(L, var.col);
            lua_rawseti(L, -2, 3);
            lua_rawseti(L, -2, i);
            i++;
        }
    } else {
        lua_pushnil(L);
    }
    return 3;
}

struct meo_stack {
    int continuation;
    meo::ast_node* node;
    int i;
    std::unique_ptr<std::vector<meo::ast_node*>> children;
    bool hasSep;
};

static int meotoast(lua_State* L) {
    size_t size = 0;
    const char* input = luaL_checklstring(L, 1, &size);
    int flattenLevel = 0;
    if (lua_isnoneornil(L, 2) == 0) {
        flattenLevel = static_cast<int>(luaL_checkinteger(L, 2));
        flattenLevel = std::max(std::min(2, flattenLevel), 0);
    }
    meo::MeoParser parser;
    auto info = parser.parse<meo::File_t>({input, size});
    if (info.node) {
        lua_createtable(L, 0, 0);
        int tableIndex = lua_gettop(L);
        lua_createtable(L, 0, 0);
        int cacheIndex = lua_gettop(L);
        auto getName = [&](meo::ast_node* node) {
            int id = node->getId();
            lua_rawgeti(L, cacheIndex, id);
            if (lua_isnil(L, -1) != 0) {
                lua_pop(L, 1);
                auto name = node->getName();
                lua_pushlstring(L, &name.front(), name.length());
                lua_pushvalue(L, -1);
                lua_rawseti(L, cacheIndex, id);
            }
        };
        std::stack<meo_stack> stack;
        auto do_call = [&](meo::ast_node* node) { stack.push({0, node, 0, nullptr, false}); };
        auto do_return = [&]() { stack.pop(); };
        do_call(info.node);
        while (!stack.empty()) {
            auto& current = stack.top();
            int continuation = current.continuation;
            auto node = current.node;
            switch (continuation) {
                case 0: {
                    if (!current.children) {
                        node->visitChild([&](meo::ast_node* child) {
                            if (meo::ast_is<meo::Seperator_t>(child)) {
                                current.hasSep = true;
                                return false;
                            }
                            if (!current.children) {
                                current.children = std::make_unique<std::vector<meo::ast_node*>>();
                            }
                            current.children->push_back(child);
                            return false;
                        });
                    }
                    current.i = 0;
                    current.continuation = 1;
                    break;
                }
                case 1: {
                    if (current.children && current.i < static_cast<int>(current.children->size())) {
                        do_call(current.children->at(current.i));
                        current.continuation = 2;
                        break;
                    }
                    current.continuation = 3;
                    break;
                }
                case 2: {
                    current.i++;
                    current.continuation = 1;
                    break;
                }
                case 3: {
                    int count = current.children ? static_cast<int>(current.children->size()) : 0;
                    switch (count) {
                        case 0: {
                            lua_createtable(L, 4, 0);
                            getName(node);
                            lua_rawseti(L, -2, 1);
                            lua_pushinteger(L, node->m_begin.m_line);
                            lua_rawseti(L, -2, 2);
                            lua_pushinteger(L, node->m_begin.m_col);
                            lua_rawseti(L, -2, 3);
                            auto str = parser.toString(node);
                            meo::Utils::trim(str);
                            lua_pushlstring(L, str.c_str(), str.length());
                            lua_rawseti(L, -2, 4);
                            lua_rawseti(L, tableIndex, lua_objlen(L, tableIndex) + 1);
                            break;
                        }
                        case 1: {
                            if (flattenLevel > 1 || (flattenLevel == 1 && !current.hasSep)) {
                                lua_rawgeti(L, tableIndex, 1);
                                getName(node);
                                lua_rawseti(L, -2, 1);
                                lua_pushinteger(L, node->m_begin.m_line);
                                lua_rawseti(L, -2, 2);
                                lua_pushinteger(L, node->m_begin.m_col);
                                lua_rawseti(L, -2, 3);
                                lua_pop(L, 1);
                                break;
                            }
                        }
                        default: {
                            auto len = lua_objlen(L, tableIndex);
                            lua_createtable(L, count + 3, 0);
                            getName(node);
                            lua_rawseti(L, -2, 1);
                            lua_pushinteger(L, node->m_begin.m_line);
                            lua_rawseti(L, -2, 2);
                            lua_pushinteger(L, node->m_begin.m_col);
                            lua_rawseti(L, -2, 3);
                            for (int i = count, j = 4; i >= 1; i--, j++) {
                                lua_rawgeti(L, tableIndex, len - i + 1);
                                lua_rawseti(L, -2, j);
                            }
                            for (int i = 1; i <= count; i++) {
                                lua_pushnil(L);
                                lua_rawseti(L, tableIndex, len);
                                len--;
                            }
                            lua_rawseti(L, tableIndex, lua_objlen(L, tableIndex) + 1);
                            break;
                        }
                    }
                    do_return();
                    break;
                }
            }
        }
        lua_rawgeti(L, tableIndex, 1);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushlstring(L, info.error.c_str(), info.error.length());
        return 2;
    }
}

static const luaL_Reg meolib[] = {{"to_lua", meotolua}, {"to_ast", meotoast}, {"version", nullptr}, {"options", nullptr}, {"load_stacktraceplus", nullptr}, {nullptr, nullptr}};

int luaopen_meo(lua_State* L) {
    luaL_newlib(L, meolib);                                              // meo
    lua_pushstring(L, "meo 0.0.1");                                      // meo version
    lua_setfield(L, -2, "version");                                      // meo["version"] = version, meo
    lua_createtable(L, 0, 0);                                            // meo options
    lua_pushlstring(L, &meo::extension.front(), meo::extension.size());  // meo options ext
    lua_setfield(L, -2, "extension");                                    // options["extension"] = ext, meo options
    lua_pushliteral(L, LUA_DIRSEP);
    lua_setfield(L, -2, "dirsep");               // options["dirsep"] = dirsep, meo options
    lua_setfield(L, -2, "options");              // meo["options"] = options, meo
    lua_pushcfunction(L, init_stacktraceplus);   // meo func1
    lua_setfield(L, -2, "load_stacktraceplus");  // meo["load_stacktraceplus"] = func1, meo
    lua_pushvalue(L, -1);                        // meo meo
    init_meoscript(L);                           // meo
    return 1;
}

}  // extern "C"
