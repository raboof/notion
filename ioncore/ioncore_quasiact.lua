--
-- ion/share/ioncore_quasiact.lua -- Frame quasiactivation support
-- 
-- Copyright (c) Tuomo Valkonen 2007-2009.
--
-- See the included file LICENSE for details.
--

local qa_source={}
local qa_target={}

local function quasi_activated(frame, src)
    local old=qa_source[frame]
    if old then
        qa_target[old]=nil
    end
    qa_source[frame]=src
    qa_target[src]=frame
    ioncore.defer(function() frame:set_grattr("quasiactive", "set") end)
end

local function quasi_inactivated(frame, src)
    qa_source[frame]=nil
    qa_target[src]=nil
    ioncore.defer(function() frame:set_grattr("quasiactive", "unset") end)
end

local function activated(src)
    local tgt=src:__return_target()
    if obj_is(tgt, "WFrame") then
        quasi_activated(tgt, src)
    elseif obj_is(tgt, "WGroup") then
        local mgr=tgt:manager()
        if obj_is(mgr, "WFrame") then
            quasi_activated(mgr, src)
        end
    end
end

local function inactivated(src)
    local tgt=qa_target[src]
    if tgt then
        quasi_inactivated(tgt, src)
        return true
    end
end

local function deinit(tgt)
    local src=qa_source[tgt]
    if src then
        qa_target[src]=nil
        qa_source[tgt]=nil
    end
end

local function quasiact_notify(reg, how)
    if how=="activated" or how=="pseudoactivated" then
        activated(reg)
    elseif how=="inactivated" or how=="pseudoinactivated" then
        inactivated(reg)
    elseif how=="set_return" then
        if reg:is_active(true--[[pseudoact--]]) then
            activated(reg)
        end
    elseif how=="unset_return" then
        inactivated(reg)
    elseif how=="deinit" then
        inactivated(reg)
        deinit(reg)
    end
end


ioncore.get_hook("region_notify_hook"):add(quasiact_notify)
