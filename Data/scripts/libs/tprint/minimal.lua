local defaults = { indent = "\t", eol="\n", assign=" = ", }

-- for inline use indent="" eol=" " assign="="
local function internal_tprint(t, lvl, cfg)
	lvl = lvl or 0
	if type(t) == "table" then
		local r={}
		r[#r+1]="{"
		lvl=lvl+1
		for k,v in pairs(t) do
			r[#r+1]= (cfg.indent or ""):rep(lvl).."["..internal_tprint(k,lvl,cfg).."]"..(cfg.assign or "=")..internal_tprint(v,lvl,cfg)..","
		end
		lvl=lvl-1
		r[#r+1]=(cfg.indent or ""):rep(lvl).."}"
		return table.concat(r, (cfg.eol or ""))
	end
	if type(t) == "string" then
		--return ("%q"):format(t)
		return '"'..t:gsub("[\"\\]", function(cap) return "\\"..cap end)..'"'
	end
	return tostring(t)
end
local function tprint(t, cfg)
	cfg = cfg or {}
	setmetatable(cfg, {__index=defaults})
	return internal_tprint(t, nil, cfg)
end
--return setmetatable(M, {__call=function(_, ...) return tprint(...) end})
return tprint
