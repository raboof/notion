--
-- Constants
--

TRUE = true
FALSE = false


--
-- Functions to help construct bindmaps
--

function submap2(kcb_, list)
    return {action = "kpress", kcb = kcb_, submap = list}
end

function submap(kcb_)
    local function submap_helper(list)
	return submap2(%kcb_, list)
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


--
-- Callback creation functions
-- 

function make_active_leaf_fn(fn)
    if not fn then
	error("fn==nil", 2)
    end
    local function call_active_leaf(reg)
	lf=region_get_active_leaf(reg)
	%fn(lf)
    end
    return call_active_leaf
end

function make_screen_switch_nth_fn(n)
    local function call_nth(scr)
        screen_switch_nth_on_cvp(scr, %n)
    end
    return call_nth
end
    
function make_exec_fn(cmd)
    local function do_exec(scr)
	return exec_on_screen(scr, %cmd)
    end
    return do_exec
end


--
-- Includes
--

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


--
-- Winprops
--

function winprop(list)
    local list2, class, role, instance = {}, "*", "*", "*"

    for k,v in list do
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
    
    add_winprop(class, role, instance, list2)
end

--
-- Misc
-- 

function obj_exists(obj)
    return (obj_typename(obj)==nil)
end
