--
-- ion/share/menulib.lua -- Helper functions for defining menus.
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
if _LOADED["menulib"] then return end

local menulib={}
_G.menulib=menulib

-- Table to hold defined menus.
local menus={}


-- Menu construction {{{

--DOC
-- Define a new menu with \var{name} being the menu's name and \var{tab} 
-- being a table of menu entries.
function menulib.defmenu(name, tab)
    menus[name]=tab
end

--DOC
-- If \var{menu_or_name} is a string, returns a menu defined
-- with \fnref{defmenu}, else return \var{menu_or_name}.
function menulib.getmenu(menu_or_name)
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
function menulib.menuentry(name, cmd, ...)
    return {name=name, cmd=cmd, args=arg}
end

--DOC
-- Use this function to define menu entries for submenus.
function menulib.submenu(name, sub_or_name)
    return {
        name=name,
        submenu_fn=function() 
                       return menulib.getmenu(sub_or_name) 
                   end
    }
end


-- }}}


-- Menu commands {{{

local function do_menu(reg, sub, menu_or_name, fn)
    local function wrapper(entry)
        if entry.cmd then
            if type(entry.cmd)=="function" then
                entry.cmd(reg, sub)
            else
                ioncorelib.do_cmd(reg, sub, entry.cmd, entry.args)
            end
        end
    end
    return fn(reg, wrapper, menulib.getmenu(menu_or_name))
end


defcmd2("WMPlex", "menu", 
        function(mplex, sub, menu_or_name) 
            return do_menu(mplex, sub, menu_or_name, menu_menu)
        end)

defcmd2("WMPlex", "bigmenu", 
        function(mplex, sub, menu_or_name) 
            local function menu_bigmenu(m, s, menu)
                return menu_menu(m, s, menu, true)
            end
            return do_menu(mplex, sub, menu_or_name, menu_bigmenu)
        end)

defcmd2("WWindow", "pmenu",
        function(win, sub, menu_or_name) 
            return do_menu(win, sub, menu_or_name, menu_pmenu)
        end)

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
    local wss=complete_region("", "WGenWS")
    table.sort(wss)
    local entries={}
    for i, name in wss do
        local ws=lookup_region(name, "WGenWS")
        entries[i]=menuentry(name, function() ws:goto() end)
    end
    
    return entries
end


-- }}}


-- Style menu {{{


local function mplex_of(reg)
    while reg and not obj_is(reg, "WMPlex") do
        reg=reg:parent()
    end
    return reg
end

local function selectstyle(look, where)
    include(look)

    local fname=get_savefile('draw')

    local function writeit()
        local f, err=io.open(fname, 'w')
        if not f then
            query_message(where, err)
        else
            f:write(string.format('include("%s")\n', look))
            f:close()
        end
    end

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
        if string.len(data)>ioncorelib.RESULT_DATA_LIMIT then
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
    
    table.insert(stylemenu, menuentry("Refresh list",
                                      menulib.refresh_styles))
    
    menus.stylemenu=stylemenu
end


--DOC
-- Refresh list of known style files.
function menulib.refresh_styles()
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

-- }}}

ioncorelib.export(menulib,
                  "defmenu",
                  "menuentry",
                  "submenu")

menulib.refresh_styles()

-- Mark ourselves loaded.
_LOADED["menulib"]=true
