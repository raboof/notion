--
-- ion/share/menulib.lua -- Helper functions for defining menus.
-- 
-- Copyright (c) Tuomo Valkonen 2003.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- This is a slight abuse of the _LOADED variable perhaps, but library-like 
-- packages should handle checking if they're loaded instead of confusing 
-- the user with require/include differences.
if _LOADED["menulib"] then return end


-- Table to hold defined menus.
local menus={}


--DOC
-- Define a new menu with \var{name} being the menu's name and \var{tab} 
-- being a table of menu entries.
function defmenu(name, tab)
    menus[name]=tab
end


--DOC
-- If \var{menu_or_name} is a string, returns a menu defined
-- with \fnref{defmenu}, else return \var{menu_or_name}.
function getmenu(menu_or_name)
    if type(menu_or_name)=="string" then
        if type(menus[menu_or_name])=="table" then
            return menus[menu_or_name]
        elseif type(menus[menu_or_name])=="function" then
            return menus[menu_or_name]()
        end
    else
        return menu_or_name
    end
end

--DOC
-- Use this function to define normal menu entries.
function menuentry(name, fn)
    return {name=name, fn=fn}
end


--DOC
-- Use this function to define menu entries for submenus.
function submenu(name, sub_or_name)
    return {
        name=name,
        submenu_fn=function() 
                       return getmenu(sub_or_name) 
                   end
    }
end


--DOC
-- This function can be used to create a wrapper to create an
-- embedded-in-an-mplex menu and to call the handler for a menu
-- entry once selected. 
-- 
-- See also: \fnref{menu_menu}, \fnref{make_bigmenu_fn} and 
-- \fnref{make_pmenu_fn}.
function make_menu_fn(menu_or_name, big)
    return function(mplex, ...)
               local params=arg
               local function wrapper(entry)
                   if entry.fn then
                       entry.fn(mplex, unpack(params))
                   end
               end
               return menu_menu(mplex, wrapper, getmenu(menu_or_name), big)
           end
end


--DOC
-- This function is similar to \fnref{make_menu_fn} but uses a possibly
-- bigger style for the menu.
function make_bigmenu_fn(menuname)
    return make_menu_fn(menuname, true)
end


--DOC
-- This function can be used to create a wrapper display a drop-down menu
-- and to call the handler for a menu entry once selected. The resulting
-- function should only be called from a mouse press binding.
-- 
-- See also: \fnref{menu_pmenu}, \fnref{make_menu_fn}.
function make_pmenu_fn(menu_or_name)
    return function(win, ...)
               local params=arg
               local function wrapper(entry)
                   if entry.fn then
                       entry.fn(win, unpack(params))
                   end
               end
               return menu_pmenu(win, wrapper, getmenu(menu_or_name))
           end
end


-- Window list

function menus.windowlist()
    local cwins=complete_clientwin("")
    table.sort(cwins)
    local entries={}
    for i, name in cwins do
        local cwin=lookup_clientwin(name)
        entries[i]=menuentry(name, function() cwin:goto() end)
    end
    
    return entries
end


-- Workspace list.

function menus.workspacelist()
    local wss=complete_workspace("")
    table.sort(wss)
    local entries={}
    for i, name in wss do
        local ws=lookup_workspace(name)
        entries[i]=menuentry(name, function() ws:goto() end)
    end
    
    return entries
end


-- Mark ourselves loaded.
_LOADED["menulib"]=true
