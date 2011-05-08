--[[
Author: Etan Reisner
Email: deryni@unreliablesource.net
Summary: Displays an WInfoWin on XkbBell events, also sets xkbbell grattr:s on the frame containing the window that triggered the event.
License: MIT
Last Updated: 2011-05-05

Copyright (c) Etan Reisner 2011
--]]

xkbbell = xkbbell or {}
if not xkbbell.timeout then
    xkbbell.timeout = 10000
end
if not xkbbell.low_threshold then
    xkbbell.low_threshold = 50
end
if not xkbbell.medium_threshold then
    xkbbell.medium_threshold = 75
end
if not xkbbell.high_threshold then
    xkbbell.high_threshold = 100
end

local bell_hook = ioncore.get_hook("xkb_bell_event")
if not bell_hook then
    dopath("xkbevents")
    bell_hook = ioncore.get_hook("xkb_bell_event")
end

local timer = ioncore.create_timer()
local screen_iws = setmetatable({}, {__mode="kv"})
local iw_hide_funs = setmetatable({}, {__mode="kv"})
local active_frames = setmetatable({}, {__mode="kv"})

local function set_hidden(iw, scr, state)
    scr = scr or ioncore.find_manager(iw, "WScreen")
    scr:set_hidden(iw, state or "set")
end

local function unset_grattrs(frame)
    frame:set_grattr("xkbbell", "unset")
    frame:set_grattr("xkbbell-low", "unset")
    frame:set_grattr("xkbbell-medium", "unset")
    frame:set_grattr("xkbbell-high", "unset")
end

local function hide_fun(iw, scr)
    return function()
        set_hidden(iw, scr)
    end
end

local function region_notify(reg, how)
    if how ~= "activated" then
        return
    end

    local frame = ioncore.find_manager(reg, "WFrame")
    if frame and active_frames[frame] then
        active_frames[frame] = nil
        unset_grattrs(frame)
    end
end

local function bell_info(tab)
    local style = "xkbbell"
    if tab.percent <= xkbbell.low_threshold then
        style = "xkbbell-low"
    elseif tab.percent <= xkbbell.medium_threshold then
        style = "xkbbell-medium"
    elseif tab.percent <= xkbbell.high_threshold then
        style = "xkbbell-high"
    end

    if tab.window then
        local frame = ioncore.find_manager(tab.window, "WFrame")
        if frame then
            local cframe = ioncore.find_manager(ioncore.current(), "WFrame")
            if frame ~= cframe then
                active_frames[frame] = true
                unset_grattrs(frame)
                frame:set_grattr(style, "set")
            end
            return
        end
        -- Fall through to setting a screen iw for this bell.
    end

    local scr = ioncore.current():screen_of()
    local iw = screen_iws[scr]
    ioncore.defer(function()
        if not iw then
            iw = scr:attach_new{
                name="xkbbell"..scr:id(),
                style=style,

                type="WInfoWin",
                hidden=true,
                unnumbered=true,
                sizepolicy="free",
                geom={x=0,y=0,w=1,h=1},
            }
            screen_iws[scr] = iw
            iw:set_text("Bell!", -1)

            iw_hide_funs[scr] = hide_fun(iw, scr)
        end

        set_hidden(iw, scr, "unset")
        timer:set(xkbbell.timeout, iw_hide_funs[scr])
    end)
end

if bell_hook then
    bell_hook:add(bell_info)
end

local hook = ioncore.get_hook("region_notify")
if hook then
    hook:add()
end
