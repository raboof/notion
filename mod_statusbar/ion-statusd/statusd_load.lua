--
-- ion/ext_statusbar/ion-statusd/statusd_load.lua
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
    interval=10*1000,
    load_hint=1,
    important_threshold=1.5,
    critical_threshold=4.0
}

local settings=table.join(statusd.get_config("load"), defaults)

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
    local st, en, load=string.find(s, 'load average:%s*(.*)')
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
    local lds={string.find(l, '^(%d+%.%d+).*(%d+%.%d+).*(%d+%.%d+)')}
    local v=tonumber(lds[settings.load_hint+2])
    local i="normal"
    print(v)
    if v then
        if v>settings.critical_threshold then
            i="critical"
        elseif v>settings.important_threshold then
            i="important"
        end
    end
    return i
end

local function update_load()
    local l=get_load()
    statusd.inform("load", l)
    statusd.inform("load_hint", get_hint(l))
    load_timer:set(settings.interval, update_load)
end

-- Init
get_load=detect_load_fn()
load_timer=statusd.create_timer()
update_load()

