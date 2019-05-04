--
-- ion/share/ioncore_ext.lua -- Ioncore Lua library
-- 
-- Copyright (c) Tuomo Valkonen 2004-2007.
--
-- See the included file LICENSE for details.
--

-- This is a slight abuse of the package.loaded variable perhaps, but
-- library-like packages should handle checking if they're loaded instead of
-- confusing the user with require/includer differences.
if package.loaded["ioncore"] then return end

-- Default modifiers
--META="Mod4+"
--ALTMETA=""

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

-- Historically, the options now called 'META' and 'ALTMETA' were called 'MOD1' and 'MOD2'.
-- The code below makes sure:
-- * the default values for those settings are 'Mod4+' (the 'windows key') and ''
-- * that setting either META/ALTMETA or MOD1/MOD2 will specify the primary and secondary meta keys
-- * that getting either META/ALTMETA or MOD1/MOD2 will return the (default or configured) primary and secondary meta keys

local function getmod(t, s)
    if s=="META" then
        return rawget(t, "MOD1") or "Mod4+"
    elseif s=="MOD1" then
        return rawget(t, "META") or "Mod4+"
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

notioncore = ioncore

-- Export some important functions into global namespace.
export(ioncore, 
       notioncore,
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
package.loaded["ioncore"]=true



local function dummy_gettext_hack()
    -- Extra translations for context menus etc. I don't want extra
    -- TR calls in the configuration files, or parsing the string 
    -- parameters to kpress etc. for translations.
    TR("Frame")
    TR("Screen")
    TR("Workspace")
    TR("Tiling")
    TR("Tiled frame")
    TR("Floating frame")
    TR("Context menu:")
    TR("Main menu:")
end
