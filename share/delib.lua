--
-- ion/share/delib.lua -- Helper functions for defining drawing engine 
-- styles
-- 
-- Copyright (c) Tuomo Valkonen 2003.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local stylecache={}

local function base_on(name, list)
    if not stylecache[list.based_on] then
        warn("Attempt to base style "..name.." on style "..list.based_on
             .." that is not defined.")
        return false
    end

    for k, v in stylecache[list.based_on] do
        if type(k)=="number" then
            table.insert(list, v)
        elseif not list[k] then
            list[k]=v
        end
    end
    
    return true
end

--DOC
-- Define a new style for the default drawing engine (that must've
-- been loaded with \fnref{gr_select_engine}.
function de_define_style(name, list)    
    if not list then
        return function(list2)
                   de_define_style(name, list2)
               end
    end
    
    if list.based_on then
        if not base_on(name, list) then
            return
        end
    end
    
    stylecache[name]=list
    
    for xscr, rootwin in root_windows() do
        if not de_do_define_style(rootwin, name, list) then
            break
        end
    end
end


--DOC 
-- Define a substyle for the default drawing engine. This function
-- is to be used in constructing style definitions for 
-- \fnref{de_define_style}.
function de_substyle(pattern, list)
    if not list then
        return function(list2)
                   substyle(pattern, list2)
               end
    end    
    
    list.substyle_pattern=pattern
    return list
end
