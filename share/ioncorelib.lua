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

-- Constants {{{

TRUE = true
FALSE = false

-- }}}


-- Some settings {{{

default_ws_type="WIonWS"
DEFAULT_MOD="Mod1+"
SECOND_MOD=""

-- If we're on SunOS, we need to remap some keys.
KEYF11, KEYF12="F11", "F12"

if os.execute('uname -s|grep "SunOS" > /dev/null')==0 then
    print("ioncorelib.lua: Uname test reported SunOS; ".. 
          "mapping F11=Sun36, F12=SunF37.")
    KEYF11, KEYF12="SunF36", "SunF37"
end

-- }}}


-- Functions to help construct bindmaps {{{

--DOC
-- Returns a function that creates a submap binding description table.
-- When the key press action \var{keyspec} occurs, Ioncore will wait for
-- a further key presse and act according to the submap.
-- For details, see section \ref{sec:bindings}.
function submap(kcb_, list)
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
        func = func_,
        area = area_,
    }
    
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


-- Program execution {{{

local function execrootw(reg, cmd)
    local rw=reg:rootwin_of()
    if rw then
        rw:exec_on(cmd)
    else
        exec(cmd)
    end
end


--DOC    
-- This function will generate another function that, depending on its
-- parameter, will call either \fnref{exec_in} or 
-- \fnref{exec_on_rootwin} to execute \var{cmd}. You should use this
-- function to bind execution commands to keys.
function make_exec_fn(cmd)
    return function(reg) exec_in(reg, cmd) end
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


-- Winprops {{{

local winprops={}

local function alternative_winprop_idents(id)
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
    local id, nm=cwin:get_ident(), (cwin:name() or "")
    local names, prop

    for c, r, i in alternative_winprop_idents(id) do
        names={}
        pcall(function() names=winprops[c][r][i] or {} end)
        local match, matchl=names[0], 0
        for name, prop in names do
            if type(name)=="string" then
                local st, en=string.find(nm, name)
                if st and en then
                    if en-st>matchl then
                        match=prop
                        matchl=en-st
                    end
                end
            end
        end
        -- no regexp match, use default winprop
        if match then
            return match
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
    if not winprops[class][role][instance] then
        winprops[class][role][instance]={}
    end
end    

local function do_add_winprop(class, role, instance, name, props)
    ensure_winproptab(class, role, instance)
    winprops[class][role][instance][name]=props
end

--DOC
-- Define a winprop. For more information, see section \ref{sec:winprops}.
function winprop(list)
    local list2, class, role, instance, name = {}, "*", "*", "*", 0

    for k, v in list do
	if k == "class" then
	    class = v
    	elseif k == "role" then
	    role = v
	elseif k == "instance" then
	    instance = v
	elseif k == "name" then
	    name = v
	else
	    list2[k] = v
	end
    end
    
    do_add_winprop(class, role, instance, name, list2)
end

-- }}}


-- Misc {{{

--DOC
-- Check that the \type{WObj} \var{obj} still exists in the C side of Ion.
function obj_exists(obj)
    return (obj_typename(obj)~=nil)
end

--DOC
-- Make \var{str} shell-safe.
function string.shell_safe(str)
    return "'"..string.gsub(str, "'", "'\\''").."'"
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


include('ioncorelib-mplexfns')


-- Backwards Compatibility {{{

make_current_fn=make_mplex_sub_fn
make_current_clientwin_fn=make_mplex_clientwin_fn

function WRegion.close_sub_or_self(reg)
    WRegion.close(reg:active_sub() or reg)
end

-- }}}


-- Mark ourselves loaded.
_LOADED["ioncorelib"]=true

