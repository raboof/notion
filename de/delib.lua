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

local function lookup_substyle(list, pattern)
    for k, v in ipairs(list) do
        if type(v)=="table" then
            if v.substyle_pattern and v.substyle_pattern==pattern then
                return v
            end
        end
    end
end

local function base_on(name, list)
    if not stylecache[list.based_on] then
        warn("Attempt to base style "..name.." on style "..list.based_on
             .." that is not defined.")
        return false
    end

    for k, v in stylecache[list.based_on] do
        if type(k)=="number" then
            if type(v)=="table" then
                if v.substyle_pattern then
                    if not lookup_substyle(list, v.substyle_pattern) then
                        table.insert(list, v)
                    end
                end
            end
        elseif not list[k] then
            list[k]=v
        end
    end
    
    return true
end

local translations={
    ["frame-tab"] = "tab-frame",
    ["frame-tab-ionframe"] = "tab-frame-ionframe",
    ["frame-tab-floatframe"] = "tab-frame-floatframe",
}

--DOC
-- Define a new style for the default drawing engine (that must've
-- been loaded with \fnref{gr_select_engine}.
function de_define_style(name, list)
    if translations[name] then
        warn('The style "'..name..'" has been renamed to "'
             ..translations[name]..'"')
        name=translations[name]
    end

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


local subtranslations={
    ["cursor"]="*-cursor",
    ["selection"]="*-selection",
}
    
--DOC 
-- Define a substyle for the default drawing engine. This function
-- is to be used in constructing style definitions for 
-- \fnref{de_define_style}.
function de_substyle(pattern, list)
    if subtranslations[pattern] then
        warn('The substyle "'..pattern..'" has been renamed to "'
             ..subtranslations[pattern]..'"')
        pattern=subtranslations[pattern]
    end
    
    if not list then
        return function(list2)
                   substyle(pattern, list2)
               end
    end    
    
    list.substyle_pattern=pattern
    return list
end
