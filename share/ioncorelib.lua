--
-- ion/share/ioncorelib.lua -- Ioncore Lua library
-- 
-- Copyright (c) Tuomo Valkonen 2003.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- Constants {{{

TRUE = true
FALSE = false

-- }}}


-- Some settings {{{

default_ws_type="WIonWS"
DEFAULT_MOD="Mod1+"
SECOND_MOD=""

-- }}}


-- Functions to help construct bindmaps {{{

--DOC
-- Returns a function that creates a submap binding description table.
-- When the key press action \var{keyspec} occurs, Ioncore will wait for
-- a further key presse and act according to the submap.
-- For details, see section \ref{sec:bindings}.
function submap(keyspec, list)
    if not list then
        return function(list)
                   return submap(keyspec, list)
               end
    end
    return {action = "kpress", kcb = kcb_, submap = list}
end

--DOC
-- Creates a binding description table for the action of pressing a key given 
-- by \var{keyspec} (with possible modifiers) to the function \var{func}.
-- For more information on bindings, see section \ref{sec:bindings}.
function kpress(keyspec, func)
    return {action = "kpress", kcb = keyspec, func = func}
end

--DOC
-- This is similar to \fnref{kpress} but after calling \var{func}, 
-- Ioncore waits for all modifiers to be released before processing
-- any further actions.
-- For more information on bindings, see section \ref{sec:bindings}.
function kpress_waitrel(keyspec, func)
    return {action = "kpress_waitrel", kcb = keyspec, func = func}
end

function mact(act_, kcb_, func_, area_)
    local ret={
    	action = act_,
	kcb = kcb_,
	func = func_
    }
    
    if area_ ~= nil then
	ret.area = area_
    end
    
    return ret
end

--DOC
-- Creates a binding description table for the action of clicking a mouse 
-- button while possible modifier keys are pressed,
-- both given by \var{buttonspec}, to the function \var{func}.
-- The optional argument \var{area} (a string or nil) depends on the
-- context.
-- For more information, see section \ref{sec:bindings}.
function mclick(buttonspec, func, area)
    return mact("mclick", buttonspec, func, area)
end

--DOC
-- Similar to \fnref{mclick} but for double-click.
-- Also see section \ref{sec:bindings}.
function mdblclick(buttonspec, func, area)
    return mact("mdblclick", buttonspec, func, area)
end

--DOC
-- Similar to \fnref{mclick} but for just pressing the mouse button.
-- Also see section \ref{sec:bindings}.
function mpress(buttonspec, func, area)
    return mact("mpress", buttonspec, func, area)
end

--DOC
-- Creates a binding description table for the action of moving the mouse
-- (or other pointing device) while the button given by \var{buttonspec}
-- is held pressed and the modifiers given by \var{buttonspec} were pressed
-- when the button was initially pressed.
-- Also see section \ref{sec:bindings}.
function mdrag(buttonspec, func, area)
    return mact("mdrag", buttonspec, func, area)
end

-- }}}


-- Callback creation functions {{{

--DOC 
-- Creates a function that accepts a \type{WMPlex} \var{mplex} as argument 
-- and that will call \var{fn} with argument as follows:
-- \begin{enumerate}
-- \item If \fnref{WMPlex.current_input}\code{(mplex)} is not nil, then
--   this object is the parameter
-- \item If \fnref{WMPlex.current}\code{(mplex)} is not nil, then
--   this object is the parameter
-- \item Otherwise if \var{self_if_not_current} is set, the \var{mplex} 
--   itself will be the parameter.
-- \end{enumerate}
function make_current_fn(fn, self_if_no_current)
    if not fn then
        warn("nil parameter to make_current_managed_fn")
        return
    end
    return function(mplex)
               local reg=mplex:current_input()
               if not reg then
                   reg=mplex:current()
               end
               if not reg then
                   if not self_if_no_current then
                       return 
                   end
                   reg=mplex
               end
               fn(reg)
           end
end

--DOC
-- Equivalent to \fnref{make_current_fn}\code{(fn, true)}.
function make_current_or_self_fn(fn)
    return make_current_fn(fn, true)
end

--DOC
-- Creates a function that accepts a \type{WMPlex} \var{mplex} as argument 
-- and that will call \var{fn} with argument:
-- \fnref{WMPlex.current}\code{(mplex)} if this is of type \type{WClientWin}.
function make_current_clientwin_fn(fn)
    if not fn then
        warn("nil parameter to make_current_clientwin_fn")
        return
    end
    return function(mplex)
               local reg=mplex:current()
               if reg and obj_is(reg, "WClientWin") then
                   fn(reg)
               end
           end
end


local function execrootw(reg, cmd)
    local rw=reg:rootwin_of()
    if rw then
        rw:exec_on(cmd)
    else
        exec_on_wm_display(cmd)
    end
end


--DOC    
-- This function will generate another function that, depending on its
-- parameter, will call either \fnref{exec_in} or 
-- \fnref{exec_on_rootwin} to execute \var{cmd}. You should use this
-- function to bind execution commands to keys.
function make_exec_fn(cmd)
    local function do_exec(reg)
        if obj_is(reg, "WGenFrame") then
            exec_in(frame, cmd)
        else
            execrootw(reg, cmd)
        end
    end
    return do_exec
end


--DOC
-- Equivalent to 
-- \fnref{exec_on_rootwin}\code{(}\fnref{WRegion.rootwin_of}\code{(where), cmd)}. 
-- This function should be overridden by scripts and other kludges that
-- want to attempt to put new windows where they belong.
function exec_in(where, cmd)
    execrootw(where, cmd)
end

-- }}}


-- Includes {{{

--DOC
-- Execute another file with Lua code.
function include(file)
    local obsolete_files={["common-frame-bindings.lua"]=true}
    if obsolete_files[file] then
        warn("Warning: "..file.." is obsolete")
    end
    local current_dir = "."
    if CURRENT_FILE ~= nil then
        local s, e, cdir, cfile=string.find(CURRENT_FILE, "(.*)([^/])");
        if cdir ~= nil then
            current_dir = cdir
        end
    end
    -- do_include is implemented in ioncore.
    do_include(file, current_dir)
end

-- }}}


-- Winprops {{{

local winprops={}

function alternative_winprop_idents(id)
    local function g()
        for _, c in {id.class, "*"} do
            for _, r in {id.role, "*"} do
                for _, i in {id.instance, "*"} do
                    coroutine.yield(c, r, i)
                end
            end
        end
    end
    return coroutine.wrap(g)
end

--DOC
-- Find winprop table for \var{cwin}.
function get_winprop(cwin)
    local id=cwin:get_ident()
    local prop
    
    for c, r, i in alternative_winprop_idents(id) do
        if pcall(function() prop=winprops[c][r][i] end) then
            if prop then
                return prop
            end
        end
    end
end

local function ensure_winproptab(class, role, instance)
    if not winprops[class] then
        winprops[class]={}
    end
    if not winprops[class][role] then
        winprops[class][role]={}
    end
end    

local function do_add_winprop(class, role, instance, props)
    ensure_winproptab(class, role, instance)
    winprops[class][role][instance]=props
end

--DOC
-- Define a winprop. For more information, see section \ref{sec:winprops}.
function winprop(list)
    local list2, class, role, instance = {}, "*", "*", "*"

    for k, v in list do
	if k == "class" then
	    class = v
    	elseif k == "role" then
	    role = v
	elseif k == "instance" then
	    instance = v
	else
	    list2[k] = v
	end
    end
    
    do_add_winprop(class, role, instance, list2)
end

-- }}}


-- Misc {{{

--DOC
-- Check that the \type{WObj} \var{obj} still exists in Ioncore.
function obj_exists(obj)
    return (obj_typename(obj)~=nil)
end

--DOC
-- Attempt to close using \fnref{WRegion.close} most recently active 
-- child (\fnref{WRegion.active_sub}) of, or if this is nil, \var{reg} itself.
function close_sub_or_self(reg)
    WRegion.close(reg:active_sub() or reg)
end

-- }}}


-- Hooks {{{

local hooks={}

--DOC
-- Call hook \var{hookname} with the remaining arguments to this function
-- as parameters.
function call_hook(hookname, ...)
    if hooks[hookname] then
        for _, fn in hooks[hookname] do
            fn(unpack(arg))
        end
    end
end

--DOC
-- Register \var{fn} as a handler for hook \var{hookname}.
function add_to_hook(hookname, fn)
    if not hooks[hookname] then
        hooks[hookname]={}
    end
    hooks[hookname][fn]=fn
end

--DOC
-- Unregister \var{fn} as a handler for hook \var{hookname}.
function remove_from_hook(hookname, fn)
    if hooks[hookname] then
        hooks[hookname][fn]=nil
    end
end

-- }}}


-- Names {{{

--DOC
-- Equivalent to \fnref{lookup_region}\code{(nam, "WGenWS")}.
function lookup_workspace(nam)
    return lookup_region(nam, "WGenWS")
end

--DOC
-- Equivalent to \fnref{complete_region}\code{(nam, "WGenWS")}.
function complete_workspace(nam)
    return complete_region(nam, "WGenWS")
end

-- }}}


