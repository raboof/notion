--
-- ion/share/ioncore_ext.lua -- Ioncore Lua library
-- 
-- Copyright (c) Tuomo Valkonen 2004-2005.
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
MOD1="Mod1+"
MOD2=""

-- Maximum number of bytes to read from pipes
ioncore.RESULT_DATA_LIMIT=1024^2

-- Bindings, winprops, hooks and extra commands
dopath('ioncore_luaext')
dopath('ioncore_bindings')
dopath('ioncore_winprops')
dopath('ioncore_misc')
dopath('ioncore_wd')

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
       "bdoc")

-- Mark ourselves loaded.
_LOADED["ioncore"]=true

