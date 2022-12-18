

local _G = _G

mlua = require("metadot_lua")

_G.loadfile = mlua.loadfile
_G.dofile = mlua.dofile
_G.loadstring = mlua.loadstring
_G.dostring = mlua.dostring

mlua.dofile("data/scripts/samples/aut.lua")
mlua.dofile("data/scripts/samples/inc.lua")