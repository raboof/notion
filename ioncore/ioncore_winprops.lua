--
-- ion/share/ioncore_winprops.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2007.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local ioncore=_G.ioncore

local winprops={}

local function ifnil(...)
    local n=#arg
    local function nxt(_, i)
        local j=i+1
        if i==n then
            return nil
        else
            local j=i+1
            if not arg[j] then
                return nxt(nil, j)
            else
                return j, arg[j]
            end
        end
    end
            
    return nxt, nil, 0
end

local function ipairs_r(tab)
    local function nxt(_, n)
        if n==1 then
            return nil
        else
            return n-1, tab[n-1]
        end
    end
    return nxt, nil, #tab+1
end

--DOC
-- Find winprop table for \var{cwin}.
function ioncore.getwinprop(cwin)
    local id=cwin:get_ident()
    local props, prop

    for _, c in ifnil(id.class, "*") do
        for _, r in ifnil(id.role, "*") do
            for _, i in ifnil(id.instance, "*") do
                --printpp(c, r, i)
                props={}
                pcall(function() props=winprops[c][r][i] or {} end)
                for idx, prop in ipairs_r(props) do
                    if prop:match(cwin) then
                        if prop.oneshot then
                            table.remove(props, idx)
                        end
                        return prop
                    end
                end
            end
        end
    end
end

ioncore.set{_get_winprop=ioncore.getwinprop}

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

local function do_add_winprop(class, role, instance, name, prop)
    ensure_winproptab(class, role, instance)
    table.insert(winprops[class][role][instance], prop)
end


--DOC
-- The basic name-based winprop matching criteria.
function ioncore.match_winprop_name(prop, cwin)
    local nm=cwin:name()
    if not prop.name then
        return true
    elseif nm then
        local st, en=string.find(nm, prop.name)
        return (st and en)
    else
        return false
    end
end


--DOC
-- Define a winprop. For more information, see section \ref{sec:winprops}.
function ioncore.defwinprop(list)
    local list2 = {}
    local class, role, instance = "*", "*", "*"
    
    for k, v in pairs(list) do
        if k == "class" then
            class = v
        elseif k == "role" then
            role = v
        elseif k == "instance" then
            instance = v
        end
        list2[k] = v
    end
    
    if not list2.match then
        list2.match=ioncore.match_winprop_name
    end
    
    do_add_winprop(class, role, instance, name, list2)
end

