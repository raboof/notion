--
-- ion/share/ioncorelib.lua -- Ioncore Lua library
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- This is a slight abuse of the _LOADED variable perhaps, but library-like 
-- packages should handle checking if they're loaded instead of confusing 
-- the user with require/includer differences.
if _LOADED["ioncorelib"] then return end

local ioncorelib={}
_G.ioncorelib=ioncorelib

-- Default modifiers
MOD1="Mod1+"
MOD2=""

-- Default workspace type
DEFAULT_WS_TYPE="WIonWS"

-- How many characters of result data to completions do we allow?
ioncorelib.RESULT_DATA_LIMIT=1024^2

-- Bindings, winprops, hooks and extra commands
include('ioncorelib-luaext')
include('ioncorelib-bindings')
include('ioncorelib-winprops')
include('ioncorelib-extras')

-- Export some important functions into global namespace.
ioncorelib.export(ioncorelib, 
                  "submap",
                  "kpress",
                  "kpress_wait",
                  "mpress",
                  "mclick",
                  "mdblclick",
                  "mdrag",
                  "defbindings",
                  "defwinprop")

--[[ioncorelib.export(ioncore,
                  "warn",
                  "exec")--]]

-- Mark ourselves loaded.
_LOADED["ioncorelib"]=true

