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
-- the user with require/include differences.
if _LOADED["ioncorelib"] then return end

local ioncorelib={}
_G.ioncorelib=ioncorelib

-- Default modifiers
DEFAULT_MOD="Mod1+"
SECOND_MOD=""

-- Default workspace type
DEFAULT_WS_TYPE="WIonWS"

-- How many characters of result data to completions do we allow?
ioncorelib.RESULT_DATA_LIMIT=1024^2

-- If we're on SunOS, we need to remap some keys.
KEYF11, KEYF12="F11", "F12"

if os.execute('uname -s|grep "SunOS" > /dev/null')==0 then
    print("ioncorelib.lua: Uname test reported SunOS; ".. 
          "mapping F11=Sun36, F12=SunF37.")
    KEYF11, KEYF12="SunF36", "SunF37"
end

-- Bindings, winprops, hooks and extra commands
include('ioncorelib-luaext')
include('ioncorelib-bindings')
include('ioncorelib-winprops')
include('ioncorelib-hooks')
include('ioncorelib-extras')

-- Export some important functions into global namespace.
ioncorelib.export(ioncorelib, 
                  "submap",
                  "kpress",
                  "kpress_waitrel",
                  "mpress",
                  "mclick",
                  "mdblclick",
                  "mdrag",
                  "defcmd",
                  "defcmd2",
                  "defbindings",
                  "winprop",
                  "get_winprop", -- C callback
                  "call_hook") -- C callback

-- Mark ourselves loaded.
_LOADED["ioncorelib"]=true

