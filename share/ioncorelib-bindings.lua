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

local ICL=ioncorelib

--DOC
-- Returns a function that creates a submap binding description table.
-- When the key press action \var{keyspec} occurs, Ioncore will wait for
-- a further key presse and act according to the submap.
-- For details, see section \ref{sec:bindings}.
function ICL.submap(kcb_, list)
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
function ICL.kpress(keyspec, cmd, ...)
    return {action = "kpress", kcb = keyspec, cmd = cmd, args=arg}
end

--DOC
-- This is similar to \fnref{kpress} but after calling \var{cmd}, 
-- Ioncore waits for all modifiers to be released before processing
-- any further actions.
-- For more information on bindings, see section \ref{sec:bindings}.
function ICL.kpress_waitrel(keyspec, cmd)
    return {action = "kpress_waitrel", kcb = keyspec, cmd = cmd}
end

local function mact(act_, kcb_, cmd_, args_)
    local st, en, kcb2_, area_=string.find(kcb_, "([^@]*)@(.*)")
    return {
    	action = act_,
	kcb = (kcb2_ or kcb_),
        cmd = cmd_,
        area = area_,
        args = args_,
    }
end

--DOC
-- Creates a binding description table for the action of clicking a mouse 
-- button while possible modifier keys are pressed,
-- both given by \var{buttonspec}, to the function \var{func}.
-- For more information, see section \ref{sec:bindings}.
function ICL.mclick(buttonspec, cmd, ...)
    return mact("mclick", buttonspec, cmd, arg)
end

--DOC
-- Similar to \fnref{mclick} but for double-click.
-- Also see section \ref{sec:bindings}.
function ICL.mdblclick(buttonspec, cmd, ...)
    return mact("mdblclick", buttonspec, cmd, arg)
end

--DOC
-- Similar to \fnref{mclick} but for just pressing the mouse button.
-- Also see section \ref{sec:bindings}.
function ICL.mpress(buttonspec, cmd, ...)
    return mact("mpress", buttonspec, cmd, arg)
end

--DOC
-- Creates a binding description table for the action of moving the mouse
-- (or other pointing device) while the button given by \var{buttonspec}
-- is held pressed and the modifiers given by \var{buttonspec} were pressed
-- when the button was initially pressed.
-- Also see section \ref{sec:bindings}.
function ICL.mdrag(buttonspec, cmd, ...)
    return mact("mdrag", buttonspec, cmd, arg)
end

local cmds2arg={}


function ICL.getcontext(context)
    local cls=_G[context]
    if cls then
        --and cls.classname==context then
        return cls
    else
        warn("Context "..context.." not known")
    end
end

    
function ICL.command_to_function(context, cmd, args)
    if args and table.getn(args)==0 then
        args=nil
    end
    
    -- Check class functions
    local cls=ICL.getcontext(context)
    if cls then 
        local fn=cls[cmd]
        if fn and cmds2arg[fn] then
            -- TODO: set flag for C side; default to single arg
            if not args then
                return fn
            else
                return function(obj, obj2) fn(obj, obj2, unpack(args)) end
            end
        elseif fn then
            if not args then
                return function(obj) fn(obj) end
            else
                return function(obj) fn(obj, unpack(args)) end
            end
        end
    end

    -- Check globally defined functions
    local fn=_G[cmd]
    if fn and not args then
        -- Cache?
        return function() fn() end
    elseif fn then
        return function() fn(unpack(args)) end
    end
end


function ICL.commands_to_functions(context, bindings)
    local bindingsout={}
    for _, b in ipairs(bindings) do
        local ins=true
        if b.submap then
            b.submap=ICL.commands_to_functions(context, b.submap)
        elseif type(b.cmd)=="string" then
            b.func=ICL.command_to_function(context, b.cmd, b.args)
            if not b.func then
                warn("Unknown command "..b.cmd.." for context "..context)
                ins=false
            end
        end
        if ins then
            table.insert(bindingsout, b)
        end
    end
    return bindingsout
end


function ICL.defbindings(context, bindings)
    local do_bind=_G["__defbindings_"..context]
    if not do_bind then
        warn("Context "..context.." not known")
        return
    end
    return do_bind(ICL.commands_to_functions(context, bindings))
end


function ICL.do_cmd(tgt, sub, cmd, args)
    local fn=ICL.command_to_function(obj_typename(tgt), cmd, args)
    if fn then
        fn(tgt, sub)
    else
        warn("Unknown command "..cmd)
    end
end


function ICL.do_cmd_t(tgt, sub, args)
    local cmdn=args[1]
    table.remove(args, 1)
    ICL.do_cmd(tgt, sub, cmdn, args)
end


function ICL.defcmd(context, name, fn)
    local cx=_G
    if context~="global" then
        cx=ICL.getcontext(context)
    end
    if cx then
        if cx[name] then
            warn("Attempt to redefine "..context.."::"..name)
        else
            cx[name]=fn
            return true
        end
    end
    return false
end


function ICL.defcmd2(context, name, fn)
    if ICL.defcmd(context, name, fn) then
        cmds2arg[fn]=true
    end
end


