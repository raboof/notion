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


-- Menu construction {{{


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


-- }}}


-- Workspace and window lists {{{


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


local RESULT_DATA_LIMIT=1024^2


-- }}}


-- Style menu {{{


local function mplex_of(reg)
    while reg and not obj_is(reg, "WMPlex") do
        reg=reg:parent()
    end
    return reg
end

function selectstyle(look, where)
    include(look)

    local function writeit()
        local f, err=io.open(fname, 'w')
        if not f then
            query_message(where, err)
        else
            f:write(string.format('include("%s")\n', look))
            f:close()
        end
    end

    local fname=get_savefile('draw')
    
    if not querylib or not query_message then
        if fname then
            writeit()
        end
        return
    end
    
    where=mplex_of(where)
    if not where then 
        return 
    end
    
    if not fname then
        query_message(where, "Cannot save selection.")
        return
    end
    
    local q=querylib.make_yesno_fn("Save look selection in "..fname.."?", 
                                   writeit)
    q(where)
end

local function receive_styles(str)
    local data=""
    
    while str do
        data=data .. str
        if string.len(data)>RESULT_DATA_LIMIT then
            error("Too much result data")
        end
        str=coroutine.yield()
    end
    
    local found={}
    local styles={}
    local stylemenu={}
    
    for look in string.gfind(data, "(look-[^\n]*)%.lua\n") do
        if not found[look] then
            found[look]=true
            table.insert(styles, look)
        end
    end
    
    table.sort(styles)
    
    for _, look in ipairs(styles) do
        local look_=look
        table.insert(stylemenu, menuentry(look,  
                                          function(where)
                                              selectstyle(look_, where)
                                          end))
    end
    
    menus.stylemenu=stylemenu
end

                          
local function refresh_styles()
    local cmd=lookup_script("ion-completefile")
    if cmd then
        local dirs=ioncore_get_scriptdirs()
        if table.getn(dirs)==0 then
            return
        end
        for _, s in dirs do
            cmd=cmd.." "..string.shell_safe(s.."/look-")
        end
        
        popen_bgread(cmd, coroutine.wrap(receive_styles))
    end
end

refresh_styles()


-- }}}


-- Mark ourselves loaded.
_LOADED["menulib"]=true
