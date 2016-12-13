-- Authors: Relu Patrascu
-- License: LGPL, version 2.1 or later
-- Last Changed: 2004-11-05
--
-- statusd_cpuspeed.lua
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

if not statusd_cpuspeed then
  statusd_cpuspeed={ interval=10*1000 }
end

local cpuspeed
local function get_cpuspeed_proc()
    local st,en
    local f=io.open('/proc/cpuinfo', 'r')
    if not f then
        return ""
    end
    local s=f:read('*a')
    f:close()
    st, en, cpuspeed = string.find(s, 'cpu MHz[%s]*:[%s]*(%d+%.%d+)')
    return tostring(cpuspeed)
end

local function get_cpuspeed_fn()
	return get_cpuspeed_proc
end

local get_cpuspeed, cpuspeed_timer

local function update_cpuspeed()
    statusd.inform("cpuspeed", get_cpuspeed())
    cpuspeed_timer:set(statusd_cpuspeed.interval, update_cpuspeed)
end

-- Init
get_cpuspeed=get_cpuspeed_fn()
cpuspeed_timer=statusd.create_timer()
update_cpuspeed()

