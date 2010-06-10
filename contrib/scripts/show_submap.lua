--[[
Author: Etan Reisner
Email: deryni@unreliablesource.net
Summary: Displays an infowin with the currently active submap.
Version: 0.1
Last Updated: 2007-07-04

Copyright (c) Etan Reisner 2007.
--]]

local sswins = setmetatable({}, {__mode="kv"})

local function sswin_set_hidden(sswin, state)
    if not sswin then
        return
    end

    local mgr = ioncore.find_manager(sswin, "WScreen")
    mgr:set_hidden(sswin, state)
end

function show_submap_win(reg, keystr)
    local scr, sswin

    scr = reg:screen_of()
    sswin = sswins[scr]
    ioncore.defer(function()
        if not sswin then
            sswin = scr:attach_new({
                                    type="WInfoWin",
                                    name="show_submap_iw"..scr:id(),
                                    hidden=true,
                                    unnumbered=true,
                                    sizepolicy="free",
                                    geom={x=0, y=0, w=1, h=1},
                                    style="show_submap",
                                   })
            sswins[scr] = sswin
        end
        sswin_set_hidden(sswin, "unset")
        sswin:set_text(keystr or "", -1)
    end)
end

function hide_submap_win()
    local scr = ioncore.current():screen_of()
    sswin_set_hidden(sswins[scr], "set")
end

function setup()
    local hook = ioncore.get_hook("ioncore_submap_ungrab_hook")

    if hook then
        hook:add(hide_submap_win)
    end

    for section, bindings in pairs(ioncore.getbindings()) do
        for _,binds in pairs(bindings) do
            if binds.submap then
                defbindings(section, {submap(binds.kcb, {submap_enter("show_submap_win(_, '"..binds.kcb.."')")})})
            end
        end
    end
end

setup()

-- vim: set sw=4 sts=4 et:
