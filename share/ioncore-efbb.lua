--
-- ion/share/ioncore-efbb.lua -- Minimal emergency fallback bindings.
-- 
-- Copyright (c) Tuomo Valkonen 2003.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local groups={}
local mappings={}

if string.find(arg[1], "g") then
    table.insert(mappings, "F2 -> xterm\nF11 -> restart_wm\nF12 -> exit_wm")
    table.insert(groups, "global_bindings")
    global_bindings{
        kpress("F2", make_exec_fn("xterm")),
        kpress("F11", restart_wm),
        kpress("F12", exit_wm),
    }
end

if string.find(arg[1], "m") then
    table.insert(mappings, "Mod1+C -> WRegion.close")
    table.insert(groups, "mplex_bindings")
    mplex_bindings{
        kpress_waitrel("Mod1+C", make_current_or_self_fn(WRegion.close)),
    }
end

if string.find(arg[1], "f") then
    table.insert(mappings, "\nMod1+K P/N -> WGenFrame.switch_next/prev")
    table.insert(groups, "genframe_bindings")
    genframe_bindings{
        submap("Mod1+K") {
            kpress("AnyModifier+N", WGenFrame.switch_next),
            kpress("AnyModifier+P", WGenFrame.switch_prev),
        }
    }
end

if table.getn(groups)>0 then
    warn("The binding group(s) " .. table.concat(groups, ", ") .. 
         " were empty so the\nfollowing emergency mappings have " ..
         "been made:\n  " .. table.concat(mappings, "\n  ") .. 
         "\nPlease fix your configuration files!")
end

