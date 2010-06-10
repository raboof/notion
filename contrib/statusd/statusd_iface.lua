-- statusd_iface.lua
-- 
-- Copyright (c) Relu Patrascu 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--
-- Nov. 5, 2004
-- Disclaimer
-- Neither am I a lua expert nor do I have the time to invest in writing
-- better code here.  I simply needed this utility and it works OK for me.
-- I give no guarantees of any kind for this code.  Suggestions for
-- improvement are welcome.
-- ikoflexer at gmail dot com

if not statusd_iface then
  statusd_iface={ interval=10*1000 }
end

local iface
local function get_iface_proc()
    local st, en
    local f=io.open('/proc/net/wireless', 'r')
    if not f then
        return ""
    end
    -- first 2 lines -- headers
    local s=f:read('*l')
    s=f:read('*l')
    -- the third line has wifi info
    s=f:read('*l')
    if not s then
		f=io.open('/proc/net/dev', 'r')
		if not f then
	   		return "net off"
		end
		-- first 2 lines -- headers, next line check for lo
    	s=f:read('*l')
    	s=f:read('*l')
    	s=f:read('*l')
    	st, en, iface = string.find(s, '(%w+:)')
		if iface == 'lo:' then
    		s=f:read('*l')
		end
		if not s then
			return "net off"
		end
    	st, en, iface = string.find(s, '(%w+:)')
    	f:close()
        return tostring(iface) 
    end
    st, en, iface = string.find(s, '(%w+:)')
    f:close()
    return tostring(iface)
end

local function get_iface_fn()
	return get_iface_proc
end

local get_iface, iface_timer

local function update_iface()
    statusd.inform("iface", get_iface())
    iface_timer:set(statusd_iface.interval, update_iface)
end

-- Init
get_iface=get_iface_fn()
iface_timer=statusd.create_timer()
update_iface()

