--[[

Description: Switch focus in the specified direction, but with the following 
difference to the usual ioncore.goto_next function:

* When switching focus down (resp. up) from the bottom (resp. upmost) client, 
switch to the next (resp. previous) workspace.

* When switching focus right (resp. left) from the rightmost (resp. leftmost) 
client, switch to the next (resp. previous) screen.

Author: Philipp Hartwig

Usage: Add bindings for
    wrap_wsscr.goto_next(_sub, 'up', {no_ascend=_})
    wrap_wsscr.goto_next(_sub, 'down', {no_ascend=_})
    wrap_wsscr.goto_next(_sub, 'right')
    wrap_wsscr.goto_next(_sub, 'left')
to the "WTiling" section of cfg_tiling.lua.

If you want support for floating workspaces, add bindings for
    wrap_wsscr.goto_next(_chld, 'up', {no_ascend=_})
    wrap_wsscr.goto_next(_chld, 'down', {no_ascend=_})
    wrap_wsscr.goto_next(_chld, 'right')
    wrap_wsscr.goto_next(_chld, 'left')
to the "WScreen" section of cfg_notioncore.lua.

--]]

wrap_wsscr={}

local function next_ws(screen)
    local cur_index=screen:get_index(screen:mx_current())
    return screen:mx_nth(cur_index+1) or screen:mx_nth(0)
end

local function prev_ws(screen)
    local cur_index=screen:get_index(screen:mx_current())
    return screen:mx_nth(cur_index-1) or screen:mx_nth(screen:mx_count()-1)
end

function wrap_wsscr.goto_next(region, direction, param)
    local workspace=region:manager()
    if not param then param={} end
    param.nowrap=true
    if not ioncore.goto_next(region, direction, param) then
        local function underlying_manager(ws)
            return ws:bottom() or ws
        end
        if direction=='left' then
            local screen=ioncore.goto_prev_screen()
            local reg=ioncore.navi_first(underlying_manager(screen:current()), 'right')
            if reg then reg:display() end
        elseif direction=='right' then
            local screen=ioncore.goto_next_screen()
            local reg=ioncore.navi_first(underlying_manager(screen:current()), 'left')
            if reg then reg:display() end
        elseif direction=='up' then
            local screen=workspace:screen_of()
            local ws=underlying_manager(prev_ws(screen))
            --For floating workspaces, ioncore.navi_first only seems to work 
            --when the workspace is already displayed. Can someone explain 
            --this?
            ws:display()
            local reg=ioncore.navi_first(ws, 'down', {no_ascend=ws,})
            if reg then reg:display() end
        elseif direction=='down' then
            local screen=workspace:screen_of()
            local ws=underlying_manager(next_ws(screen))
            --For floating workspaces, ioncore.navi_first only seems to work 
            --when the workspace is already displayed. Can someone explain 
            --this?
            ws:display()
            local reg=ioncore.navi_first(ws, 'up', {no_ascend=ws,})
            if reg then reg:display() end
        end
    end
end
