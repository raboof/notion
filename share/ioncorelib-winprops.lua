--
-- ion/share/ioncorelib-winprops.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local ioncorelib=_G.ioncorelib

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
function ioncorelib.get_winprop(cwin)
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
function ioncorelib.winprop(list)
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

