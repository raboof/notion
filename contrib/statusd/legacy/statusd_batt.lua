-- Authors: Relu Patrascu
-- License: LGPL, version 2.1 or later
-- Last Changed: 2004-11-05
--
-- statusd_batt.lua
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

if not statusd_batt then
  statusd_batt={ interval=10*1000 }
end

local battInfoStr
local function get_batt_procapm()
    local f=io.open('/proc/apm', 'r')
    if not f then
        return ""
    end
    local s=f:read('*l')
    f:close()
    local st, en, battInfoStr = string.find(s, '(%d+%% [-]*%d+ [%?]*[%a]*)$')
    battInfoStr = string.gsub(battInfoStr, '-1.*',"-(= ")
    return tostring(battInfoStr)
end

local function get_batt_fn()
	return get_batt_procapm
end

local get_batt, batt_timer

local function update_batt()
    statusd.inform("batt", get_batt())
    batt_timer:set(statusd_batt.interval, update_batt)
end

-- Init
get_batt=get_batt_fn()
batt_timer=statusd.create_timer()
update_batt()

