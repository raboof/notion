-- Authors: Tuomo Valkonen, Etan Reisner
-- License: LGPL, version 2.1 or later
-- Last Changed: 2007
--
-- statusbar_act.lua
--
-- Copyright (c) Tuomo Valkonen 2006.
-- Copyright (c) Etan Reisner 2007.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local function notifyact()
    local act = ioncore.activity_first()
    local actcount = 0

    if act then
        mod_statusbar.inform('act_first', act:name())
        mod_statusbar.inform('act_first_hint', 'activity')
        local function count_act(reg)
            actcount = actcount + 1
            return true
        end
        ioncore.activity_i(count_act)
        mod_statusbar.inform('act_n', tostring(actcount))
        mod_statusbar.inform('act_n_hint', 'activity')
    else
        mod_statusbar.inform('act_first', '--')
        mod_statusbar.inform('act_first_hint', 'normal')
        mod_statusbar.inform('act_n', '0')
        mod_statusbar.inform('act_n_hint', 'normal')
    end
end

local function hookhandler(reg, how)
    if how ~= "activity" then
        return
    end

    ioncore.defer(function() notifyact() mod_statusbar.update() end)
end

ioncore.get_hook('region_notify_hook'):add(hookhandler)

notifyact()
mod_statusbar.inform('act_first_template', 'Mxxxxxxxxxxxxxx')
mod_statusbar.update(true)
