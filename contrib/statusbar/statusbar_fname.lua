-- Authors: Emanuele Giaquinta
-- License: Unknown
-- Last Changed: 2007
--
-- Copyright (c) Emanuele Giaquinta 2007

local function get_fname()
    local name=""
    local cur=ioncore.find_manager(ioncore.current(), "WFrame")
    if cur ~= nil then
        name = cur:name()
    end

    mod_statusbar.inform('fname', name)
end

local function hookhandler(reg, how)
    ioncore.defer(function() get_fname() mod_statusbar.update() end)
end

ioncore.get_hook("region_notify_hook"):add(hookhandler)
