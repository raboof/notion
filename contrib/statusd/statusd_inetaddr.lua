-- Authors: Relu Patrascu <ikoflexer@gmail.com>
-- License: LGPL, version 2.1 or later
-- Last Changed: 2004-11-05
--
-- statusd_inetaddr.lua
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

if not statusd_inetaddr then
  statusd_inetaddr={ interval=10*1000 }
end

local function get_iface_proc()
    local st, en, iface
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

local function get_inetaddr_ifcfg()
	local inetaddr
	local st,en,iface = string.find(tostring(get_iface_proc()), '(%w+):')
	local f = io.popen("/sbin/ifconfig " .. iface)
	if not f then
		inetaddr=""
	else
		local ifconfig_info = f:read('*a')
		f:close()
		st,en,inetaddr = string.find(ifconfig_info, 'inet addr:(%d+\.%d+\.%d+\.%d+)')
		if not inetaddr then
			inetaddr=""
		end
	end
	return tostring(" " .. inetaddr)
end

local function get_inetaddr_fn()
	return get_inetaddr_ifcfg
end

local get_inetaddr, inetaddr_timer

local function update_inetaddr()
    statusd.inform("inetaddr", get_inetaddr())
    inetaddr_timer:set(statusd_inetaddr.interval, update_inetaddr)
end

-- Init
get_inetaddr=get_inetaddr_fn()
inetaddr_timer=statusd.create_timer()
update_inetaddr()

