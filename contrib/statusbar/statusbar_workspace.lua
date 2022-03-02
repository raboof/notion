-- Authors: Rico Schiekel <fire@paranetic.de>, Canaan Hadley-Voth, Kevin Granade <kevin.granade@gmail.com>, Akaky Chertyhansky <akakychert@gmail.com>
-- License: Unknown
-- Last Changed: 2022-02-23

-- statusbar_workspace.lua
--
-- Show current workspace name in the statusbar.
-- 
-- Put any of these in cfg_statusbar.lua's template-line:
--  %workspace_name1
--  %workspace_name2
--
-- This is an internal statusbar monitor and does NOT require
-- a dopath statement (effective after a 2006-02-12 build).
--
-- version 1
-- author: Rico Schiekel <fire at paranetic dot de>
--
-- version 2
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
-- 
-- version 4
-- update for notion-4.0.2 on 2022-02-23
-- to work with mod_xinerama and two screens
-- by Akaky Chertyhansky <akakychert@gmail.com >
-- (it's a dogshit, but it works)

local function update_workspace()
    local scr1=ioncore.find_screen_id(0)
    local scr2=ioncore.find_screen_id(1)
    local curws1 = scr1:mx_current()
    local curws2 = scr2:mx_current()
    local wstype, c
    local curindex1 = scr1:get_index(curws1)+1
    local curindex2 = scr2:get_index(curws2)+1
    

    local fr,cur


    ioncore.defer( function()
	mod_statusbar.inform('workspace_name1', curws1:name())
	mod_statusbar.inform('workspace_name2', curws2:name())
	mod_statusbar.update()
    end)
end

local function update_workspace_wrap(reg, how)
    if how ~= "name" then
        return
    end

    update_workspace()
end

ioncore.get_hook("region_notify_hook"):add(update_workspace_wrap)
ioncore.get_hook("screen_managed_changed_hook"):add(update_workspace)
