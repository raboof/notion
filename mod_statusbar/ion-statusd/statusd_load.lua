--
-- ion/mod_statusbar/ion-statusd/statusd_load.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2005.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

--
-- We should really use getloadavg(3) instead and move the meter to
-- Ion side to get properly up-to-date loads. But until such an export
-- is made, and we use potentially blocking files and external programs, 
-- this meter must be in ion-statusd.
--

local defaults={
    update_interval=10*1000,
    load_hint=1,
    important_threshold=1.5,
    critical_threshold=4.0
}

local settings=table.join(statusd.get_config("load"), defaults)

local loadpat='^(%d+%.%d+).*(%d+%.%d+).*(%d+%.%d+)'

local function get_load_proc()
    local f=io.open('/proc/loadavg', 'r')
    if not f then
        return ""
    end
    local s=f:read('*l')
    f:close()
    local st, en, load=string.find(s, '^(%d+%.%d+ %d+%.%d+ %d+%.%d+)')
    
    return string.gsub((load or ""), " ", ", ")
end

local function get_load_uptime()
    local f=io.popen('uptime', 'r')
    if not f then
        return "??"
    end
    local s=f:read('*l')
    f:close()
    local st, en, load=string.find(s, 'load averages?:%s*(.*)')
    return (load or "")
end

local function detect_load_fn()
    if get_load_proc()~="" then
        return get_load_proc
    else
        return get_load_uptime
    end
end

local get_load, load_timer

local function get_hint(l)
    local v=tonumber(l)
    local i="normal"
    if v then
        if v>settings.critical_threshold then
            i="critical"
        elseif v>settings.important_threshold then
            i="important"
        end
    end
    return i
end

local l1min, l5min, l15min=2+1, 2+2, 2+3

local function update_load()
    local l = get_load()    
    local lds={string.find(l, loadpat)}
    statusd.inform("load", l)
    statusd.inform("load_hint", get_hint(l, lds[settings.load_hint+2]))
    statusd.inform("load_1min", lds[l1min])
    statusd.inform("load_1min_hint", get_hint(lds[l1min]))
    statusd.inform("load_5min", lds[l5min])
    statusd.inform("load_5min_hint", get_hint(lds[l5min]))
    statusd.inform("load_15min", lds[l15min])
    statusd.inform("load_15min_hint", get_hint(lds[l15min]))
    load_timer:set(settings.update_interval, update_load)
end

-- Init
--statusd.inform("load_template", "0.00, 0.00, 0.00");

get_load=detect_load_fn()
load_timer=statusd.create_timer()
update_load()


