--
-- Ioncore Lua library
-- 


-- {{{ Constants

TRUE = true
FALSE = false

-- }}}


-- {{{ Functions to help construct bindmaps

function submap2(kcb_, list)
    return {action = "kpress", kcb = kcb_, submap = list}
end

--DOC
-- Returns a function that creates a submap binding description table.
-- When the key press action \var{keyspec} occurs, Ioncore will wait for
-- a further key presse and act according to the submap.
-- For details, see section \ref{sec:bindings}.
function submap(keyspec)
    local function submap_helper(list)
	return submap2(kcb_, list)
    end
    return submap_helper
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


-- {{{ Callback creation functions

--DOC
-- Create a function that will call \var{fn} with argument
-- \fnref{region_get_active_leaf}\code{(param)} where \var{param} is
-- the parameter to the created function.
function make_active_leaf_fn(fn)
    if not fn then
	error("fn==nil", 2)
    end
    local function call_active_leaf(reg)
	lf=region_get_active_leaf(reg)
	fn(lf)
    end
    return call_active_leaf
end


--DOC    
-- Create a function that will make the call
-- \fnref{exec_on_screen}\code{(scr, cmd)} where \var{scr} is is the
-- argument of the generated function and should be of type \type{WScreen}.
function make_exec_fn(cmd)
    local function do_exec(scr)
	return exec_on_screen(scr, cmd)
    end
    return do_exec
end



-- {{{ Includes

--DOC
-- Execute another file with Lua code.
function include(file)
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


-- {{{ Winprops

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
    local id=clientwin_get_ident(cwin)
    
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


-- {{{ Misc

--DOC
-- Check that the \type{WObj} \var{obj} still exists in Ioncore.
function obj_exists(obj)
    return (obj_typename(obj)==nil)
end

-- }}}


-- {{{ Hooks

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
