--
-- ion/share/ioncore_tabnum.lua -- Ioncore tab numbering support
-- 
-- Copyright (c) Tuomo Valkonen 2007.
--
-- See the included file LICENSE for details.
--

ioncore.tabnum={}

local framestate={}

--DOC
-- Show tab numbers on \var{frame}, clearing them when submap 
-- grab is released the next time.
function ioncore.tabnum.show(frame)
    frame:set_grattr('numbered', 'set')
    framestate[frame]='set'
end

--DOC
-- Show tab numbers on \var{frame} after a delay, clearing them
-- when submap grab is released the next time. If the numbers
-- have not been shown before this, they will not be shown.
-- The \var{delay} is in milliseconds and defaults to 250.
function ioncore.tabnum.delayed_show(frame, delay)
    local tmr=ioncore.create_timer()
    
    framestate[frame]=tmr
    
    tmr:set(delay or 250, function() ioncore.tabnum.show(frame) end)
end

--DOC
-- Clear all tab numbers set by \fnref{ioncore.tabnum.show}
-- or \fnref{ioncore.tabnum.delayed_show}.
function ioncore.tabnum.clear()
    local st=framestate
    framestate={}
    
    for f, s in pairs(st) do
        if s=='set' then
            f:set_grattr('numbered', 'unset')
        elseif obj_is(s, "WTimer") then
            s:reset()
        end
    end
end

ioncore.get_hook("ioncore_submap_ungrab_hook")
    :add(ioncore.tabnum.clear)
