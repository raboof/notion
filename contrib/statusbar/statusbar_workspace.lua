-- statusbar_workspace.lua
--
-- Show current workspace name or number in the statusbar.
-- 
-- Put any of these in cfg_statusbar.lua's template-line:
--  %workspace_name
--  %workspace_frame
--  %workspace_pager
--  %workspace_name_pager
--  %workspace_num_name_pager
--
-- This is an internal statusbar monitor and does NOT require
-- a dopath statement (effective after a 2006-02-12 build).
--
-- version 1
-- author: Rico Schiekel <fire at paranetic dot de>
--
-- version 2
-- added 2006-02-14 by Canaan Hadley-Voth:
--  * %workspace_pager shows a list of workspace numbers 
--    with the current one indicated:
--
--    1i  2i  [3f]  4p  5c
--
--    i=WIonWS, f=WFloatWS, p=WPaneWS, c=WClientWin/other
--
--  * %workspace_frame - name of the active frame.
--
--  * Added statusbar_ to the filename (since it *is*
--    an internal statusbar monitor) so that it works without
--    a "dopath" call.
--
--  * Removed timer.  Only needs to run on hook.
--    Much faster this way.
--
-- version 3
-- update for ion-3rc-20070506 on 2007-05-09 
-- by Kevin Granade <kevin dot granade at gmail dot com>
--
-- Updated to use new wx_ api
-- Replaced region_activated_hook with region_notify_hook
-- Added %workspace_name_pager, which works similarly to %workspace_pager,
--   but instead displays the name of each workspace
-- Added display for WGroupWS to %workspace_pager, displayed as 'g'
-- 

local function update_frame()
    local fr
    ioncore.defer( function() 
	local cur=ioncore.current()
	if obj_is(cur, "WClientWin") and
	  obj_is(cur:parent(), "WMPlex") then
	    cur=cur:parent()
	end
	fr=cur:name()
	mod_statusbar.inform('workspace_frame', fr)
	mod_statusbar.update()
    end)
end

local function update_workspace()
    local scr=ioncore.find_screen_id(0)
    local curws = scr:mx_current()
    local wstype, c
    local pager=""
    local name_pager=""
    local name_pager_plus=""
    local curindex = scr:get_index(curws)+1
    n = scr:mx_count(1)
    for i=1,n do
        tmpws=scr:mx_nth(i-1)
        wstype=obj_typename(tmpws)
	if wstype=="WIonWS" then
	    c="i"
	elseif wstype=="WFloatWS" then
	    c="f"
	elseif wstype=="WPaneWS" then
	    c="p"
	elseif wstype=="WGroupWS" then
	    c="g"
	else
	    c="c"
	end
	if i==curindex then
            name_pager_plus=name_pager_plus.." ["..tmpws:name().."]"
            name_pager=name_pager.." ["..tmpws:name().."]"
	    pager=pager.." ["..(i)..c.."] "
	else
            name_pager_plus=name_pager_plus.." "..(i)..":"..tmpws:name()
            name_pager=name_pager.." "..tmpws:name()
	    pager=pager.." "..(i)..c.." "
	end
    end

    local fr,cur

    -- Older versions without an ioncore.current() should
    -- skip update_frame.
    update_frame()

    ioncore.defer( function()
	mod_statusbar.inform('workspace_pager', pager)
	mod_statusbar.inform('workspace_name', curws:name())
        mod_statusbar.inform('workspace_name_pager', name_pager)
        mod_statusbar.inform('workspace_num_name_pager', name_pager_plus)
	mod_statusbar.update()
    end)
end

ioncore.get_hook("region_notify_hook"):add(update_workspace)
