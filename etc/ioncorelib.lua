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

function submap(kcb_)
    local function submap_helper(list)
	return submap2(kcb_, list)
    end
    return submap_helper
end

function kpress(kcb_, func_)
    return {action = "kpress", kcb = kcb_, func = func_}
end

function kpress_waitrel(kcb_, func_)
    return {action = "kpress_waitrel", kcb = kcb_, func = func_}
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

function mclick(kcb_, func_, area_)
    return mact("mclick", kcb_, func_, area_)
end

function mdrag(kcb_, func_, area_)
    return mact("mdrag", kcb_, func_, area_)
end

function mdblclick(kcb_, func_, area_)
    return mact("mdblclick", kcb_, func_, area_)
end

function mpress(kcb_, func_, area_)
    return mact("mpress", kcb_, func_, area_)
end

-- }}}


-- {{{ Callback creation functions

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

function make_screen_switch_nth_fn(n)
    local function call_nth(scr)
        screen_switch_nth_on_cvp(scr, n)
    end
    return call_nth
end
    
function make_exec_fn(cmd)
    local function do_exec(scr)
	return exec_on_screen(scr, cmd)
    end
    return do_exec
end



-- {{{ Includes

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

function ensure_winproptab(class, role, instance)
    if not winprops[class] then
        winprops[class]={}
    end
    if not winprops[class][role] then
        winprops[class][role]={}
    end
end    

function do_add_winprop(class, role, instance, props)
    ensure_winproptab(class, role, instance)
    winprops[class][role][instance]=props
end

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

function obj_exists(obj)
    return (obj_typename(obj)==nil)
end

-- }}}


-- {{{ Hooks

local hooks={}

function call_hook(hookname, ...)
    if hooks[hookname] then
        for _, fn in hooks[hookname] do
            fn(unpack(arg))
        end
    end
end

function add_to_hook(hookname, fn)
    if not hooks[hookname] then
        hooks[hookname]={}
    end
    hooks[hookname][fn]=fn
end

function remove_from_hook(hookname, fn)
    if hooks[hookname] then
        hooks[hookname][fn]=nil
    end
end

-- }}}
