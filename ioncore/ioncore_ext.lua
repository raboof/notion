--
-- ion/share/ioncore_ext.lua -- Ioncore Lua library
-- 
-- Copyright (c) Tuomo Valkonen 2004-2006.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- This is a slight abuse of the _LOADED variable perhaps, but library-like 
-- packages should handle checking if they're loaded instead of confusing 
-- the user with require/includer differences.
if _LOADED["ioncore"] then return end

-- Default modifiers
--MOD1="Mod1+"
--MOD2=""

-- Maximum number of bytes to read from pipes
ioncore.RESULT_DATA_LIMIT=1024^2

-- Bindings, winprops, hooks, menu database and extra commands
dopath('ioncore_luaext')
dopath('ioncore_bindings')
dopath('ioncore_winprops')
dopath('ioncore_misc')
dopath('ioncore_wd')
dopath('ioncore_menudb')

-- Modifier setup compatibility kludge
local oldindex

local function getmod(t, s)
    if s=="META" then
        return rawget(t, "MOD1") or "Mod1+"
    elseif s=="MOD1" then
        return rawget(t, "META") or "Mod1+"
    elseif s=="ALTMETA" then
        return rawget(t, "MOD2") or ""
    elseif s=="MOD2" then
        return rawget(t, "ALTMETA") or ""
    elseif oldindex then
        return oldindex(t, s)
    end
end

local oldmeta, newmeta=getmetatable(_G), {}
if oldmeta then
    newmeta=table.copy(oldmeta)
    oldindex=oldmeta.__index
end
newmeta.__index=getmod
setmetatable(_G, newmeta)

-- Export some important functions into global namespace.
export(ioncore, 
       "submap",
       "kpress",
       "kpress_wait",
       "mpress",
       "mclick",
       "mdblclick",
       "mdrag",
       "defbindings",
       "defwinprop",
       "warn",
       "exec",
       "TR",
       "bdoc",
       "defmenu",
       "defctxmenu",
       "menuentry",
       "submenu")

-- Mark ourselves loaded.
_LOADED["ioncore"]=true



local function dummy_gettext_hack()
    -- For context menu labels in configuration files. 
    -- Alternative would be to hack lxgettext to support parsing
    -- the second argument to a function, or using TR in the
    -- configuration files, but I don't want the latter.
    TR("Frame")
    TR("Workspace")
end

