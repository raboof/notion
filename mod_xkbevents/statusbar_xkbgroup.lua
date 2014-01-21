--[[
Author: Etan Reisner
Email: deryni@unreliablesource.net
Summary: Adds an xkbgroup statusbar meter.
License: MIT
Last Updated: 2011-05-09

Copyright (c) Etan Reisner 2011
--]]

local group_hook = ioncore.get_hook("xkb_group_event")
if not group_hook then
    pcall(dopath, "mod_xkbevents")
    group_hook = ioncore.get_hook("xkb_group_event")
    if not group_hook then
        warn("Could not load mod_xkbevents.")
        return
    end
end

local function group_event(tab)
    if tab.group then
        mod_statusbar.inform("xkbgroup", tostring(tab.group))
    end
    ioncore.defer(mod_statusbar.update)
end

group_hook:add(group_event)
