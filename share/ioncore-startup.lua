--
-- ion/share/ioncore-startup.lua
-- 
-- Copyright (c) Tuomo Valkonen 2003.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local set_bindings={}

-- Wrap some binding setup functions so we can set up minimal bindings
-- if the configuration files fail to set up any.

local function wrap_bindings(name)
    local origfn=_G[name]
    local function wrapper(list)
        if not origfn(list) then
            return false
        end
        set_bindings[name]=true
        -- This wrapper is no longer needed, but we can't restore _G[name]
        -- if someone else is also wrapping it.
        if _G[name]==wrapper then
            _G[name]=origfn
        end
        return true
    end
    _G[name]=wrapper
end

wrap_bindings("global_bindings")
wrap_bindings("mplex_bindings")
wrap_bindings("genframe_bindings")


-- Execute configuration files

if arg and arg[1] then
    include(arg[1])
else
    include("ioncore.lua")
end


-- Set up minimal bindings if some binding group was not defined.

local mappings={}
local groups={}

if not set_bindings.global_bindings then
    table.insert(mappings, "F2 -> xterm\nF11 -> restart_wm\nF12 -> exit_wm")
    table.insert(groups, "global_bindings")
    global_bindings{
        kpress("F2", make_exec_fn("xterm")),
        kpress("F11", restart_wm),
        kpress("F12", exit_wm),
    }
end

if not set_bindings.mplex_bindings then
    table.insert(mappings, "Mod1+C -> region_close")
    table.insert(groups, "mplex_bindings")
    mplex_bindings{
        kpress_waitrel("Mod1+C", make_current_or_self_fn(region_close)),
    }
end

if not set_bindings.genframe_bindings then
    table.insert(mappings, "\nMod1+K P/N -> genframe_switch_next/prev")
    table.insert(groups, "genframe_bindings")
    genframe_bindings{
        submap("Mod1+K") {
            kpress("AnyModifier+N", genframe_switch_next),
            kpress("AnyModifier+P", genframe_switch_prev),
        }
    }
end

if table.getn(groups)>0 then
    warn("The binding group(s) " .. table.concat(groups, ", ") .. 
         " were undefined so the\nfollowing emergency mappings have " ..
         "been made:\n  " .. table.concat(mappings, "\n  ") .. 
         "\nPlease fix your configuration files!")
end

