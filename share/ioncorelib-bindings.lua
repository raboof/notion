--
-- ion/share/ioncorelib-bindings.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local ioncorelib=_G.ioncorelib

local warn=ioncore.warn

function ioncorelib.compile_cmd(cmd, guard)
    local guardcode=""
    if guard then
        local st, en, condition=string.find(guard, "^_sub:([%w-_]+)$")
        if not condition then
            warn("Invalid guard "..guard)
        elseif condition=="non-nil" then
            guardcode='if not _sub then return end; '
        else
            guarcode='if not obj_is(_sub, "'..condition..'") then return end; '
        end
    end

    local gfncode="return function(_, _sub) "..guardcode.." return true end"
    local gfn, gerr=loadstring(gfncode)
    if not gfn then
        warn("Error compiling guard: "..gerr)
    end
    gfn=gfn()
    
    if type(cmd)=="string" then
        local fncode="return function(_, _sub) "..guardcode..cmd.." end"
        local fn, err=loadstring(fncode)
        if not fn then
            warn("Error in command string: "..err)
            return
        end
        return fn(), gfn
    elseif type(cmd)=="function" then
        if gfn then
            return function(_, _sub) 
                       if gfn(_, _sub) then cmd(_, _sub) end 
                   end, gfn
        end
        return cmd
    end

    warn("Invalid command")
end


local function putcmd(cmd, guard, tab)
    local func=ioncorelib.compile_cmd(cmd, guard)
    if type(func)~="function" then
        return
    end
    
    tab.func=func
    tab.cmdstr=cmd
    tab.guard=guard
    
    return tab
end

--DOC
-- Returns a function that creates a submap binding description table.
-- When the key press action \var{keyspec} occurs, Ioncore will wait for
-- a further key presse and act according to the submap.
-- For details, see section \ref{sec:bindings}.
function ioncorelib.submap(kcb_, list)
    if not list then
        return function(lst)
                   return submap(kcb_, lst)
               end
    end
    return {action = "kpress", kcb = kcb_, submap = list}
end

--DOC
-- Creates a binding description table for the action of pressing a key given 
-- by \var{keyspec} (with possible modifiers) to the function \var{func}.
-- For more information on bindings, see section \ref{sec:bindings}.
function ioncorelib.kpress(keyspec, cmd, guard)
    return putcmd(cmd, guard, {action = "kpress", kcb = keyspec})
end

--DOC
-- This is similar to \fnref{kpress} but after calling \var{cmd}, 
-- Ioncore waits for all modifiers to be released before processing
-- any further actions.
-- For more information on bindings, see section \ref{sec:bindings}.
function ioncorelib.kpress_wait(keyspec, cmd, guard)
    return putcmd(cmd, guard, {action = "kpress_waitrel", kcb = keyspec})
end

local function mact(act_, kcb_, cmd, guard)
    local st, en, kcb2_, area_=string.find(kcb_, "([^@]*)@(.*)")
    return putcmd(cmd, guard, {
    	action = act_,
	kcb = (kcb2_ or kcb_),
        area = area_,
    })
end

--DOC
-- Creates a binding description table for the action of clicking a mouse 
-- button while possible modifier keys are pressed,
-- both given by \var{buttonspec}, to the function \var{func}.
-- For more information, see section \ref{sec:bindings}.
function ioncorelib.mclick(buttonspec, cmd, guard)
    return mact("mclick", buttonspec, cmd, guard)
end

--DOC
-- Similar to \fnref{mclick} but for double-click.
-- Also see section \ref{sec:bindings}.
function ioncorelib.mdblclick(buttonspec, cmd, guard)
    return mact("mdblclick", buttonspec, cmd, guard)
end

--DOC
-- Similar to \fnref{mclick} but for just pressing the mouse button.
-- Also see section \ref{sec:bindings}.
function ioncorelib.mpress(buttonspec, cmd, guard)
    return mact("mpress", buttonspec, cmd, guard)
end

--DOC
-- Creates a binding description table for the action of moving the mouse
-- (or other pointing device) while the button given by \var{buttonspec}
-- is held pressed and the modifiers given by \var{buttonspec} were pressed
-- when the button was initially pressed.
-- Also see section \ref{sec:bindings}.
function ioncorelib.mdrag(buttonspec, cmd, guard)
    return mact("mdrag", buttonspec, cmd, guard)
end

function ioncorelib.defbindings(context, bindings)
    local do_bind=_G['__defbindings_'..context]
    if not do_bind then
        warn("Context "..context.." not known")
        return
    end
    return do_bind(bindings)
end


