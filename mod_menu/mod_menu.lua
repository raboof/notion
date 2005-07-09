--
-- ion/mod_menu/mod_menu.lua -- Helper functions for defining menus.
-- 
-- Copyright (c) Tuomo Valkonen 2004-2005.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- This is a slight abuse of the _LOADED variable perhaps, but library-like 
-- packages should handle checking if they're loaded instead of confusing 
-- the user with require/include differences.
if _LOADED["mod_menu"] then return end

if not ioncore.load_module("mod_menu") then
    return
end

local mod_menu=_G["mod_menu"]

assert(mod_menu)

-- Table to hold defined menus.
local menus={}


-- Menu construction {{{

--DOC
-- Define a new menu with \var{name} being the menu's name and \var{tab} 
-- being a table of menu entries.
function mod_menu.defmenu(name, tab)
    menus[name]=tab
end

--DOC
-- Returns a menu defined with \fnref{mod_menu.defmenu}.
function mod_menu.getmenu(name)
    return menus[name]
end

--DOC
-- Define context menu for context \var{ctx}, \var{tab} being a table 
-- of menu entries.
function mod_menu.defctxmenu(ctx, tab)
    menus["ctxmenu-"..ctx]=tab
end

--DOC
-- Returns a context menu defined with \fnref{mod_menu.defctxmenu}.
function mod_menu.getctxmenu(name)
    return menus["ctxmenu-"..name]
end


function mod_menu.evalmenu(menu, args)
    if type(menu)=="string" then
        return mod_menu.evalmenu(menus[menu], args)
    elseif type(menu)=="function" then
        if args then
            return menu(unpack(args))
        else
            return menu()
        end
    elseif type(menu)=="table" then
        return menu
    end
end


--DOC
-- Use this function to define normal menu entries. The string \var{name} 
-- is the string shown in the visual representation of menu, and the
-- parameter \var{cmd} and \var{guard} are similar to those of
-- \fnref{ioncore.defbindings}.
function mod_menu.menuentry(name, cmd, guard)
    local fn, gfn=ioncore.compile_cmd(cmd, guard)
    if fn then
        return {name=ioncore.gettext(name), func=fn, guard_func=gfn}
    end
end

--DOC
-- Use this function to define menu entries for submenus. The parameter
-- \fnref{sub_or_name} is either a table of menu entries or the name
-- of an already defined menu. The initial menu entry to highlight can be
-- specified in \var{initial} as either an integer starting from 1, or a
-- function that returns such a number.
function mod_menu.submenu(name, sub_or_name, initial)
    return {
        name=ioncore.gettext(name),
        initial=initial or 0,
        submenu_fn=function()
                       return mod_menu.evalmenu(sub_or_name)
                   end
    }
end


-- }}}


-- Menu commands {{{

local function menu_(reg, sub, menu_or_name, fn, check)
    if check then
        -- Check that no other menus are open in reg.
        local l=reg:llist(2)
        for i, r in l do
            if obj_is(r, "WMenu") then
                return
            end
        end
    end

    menu=mod_menu.evalmenu(menu_or_name, {reg, sub})
    
    return fn(reg, function(e) e.func(reg, sub) end, menu)
end


--DOC
-- Display a menu in the lower-left corner of \var{mplex}.
-- The variable \var{menu_or_name} is either the name of a menu
-- defined with \fnref{mod_menu.defmenu} or directly a table similar
-- to ones passesd to this function. When this function is
-- called from a binding handler, \var{sub} should be set to
-- the second argument of to the binding handler (\var{_sub})
-- so that the menu handler will get the same parameters as the
-- binding handler. Extra options can be passed in the table
-- \var{param}. The initial entry can be specified as the field
-- \var{initial} as an integer starting from 1. Menus can be made
-- to use a bigger style by setting the field \var{big} to \code{true}.
function mod_menu.menu(mplex, sub, menu_or_name, param) 
   local function menu_stdmenu(m, s, menu)
      return mod_menu.do_menu(m, s, menu, param)
   end
   return menu_(mplex, sub, menu_or_name, menu_stdmenu, true)
end

-- Compatibility
function mod_menu.bigmenu(mplex, sub, menu_or_name, initial) 
    local function menu_bigmenu(m, s, menu)
      return mod_menu.do_menu(m, s, menu, {big=true, initial=initial})
    end
    return menu_(mplex, sub, menu_or_name, menu_bigmenu, true)
end

--DOC
-- This function is similar to \fnref{mod_menu.menu}, but input
-- is grabbed and \var{param.key} is used to cycle through the menu.
function mod_menu.grabmenu(mplex, sub, menu_or_name, param) 
    if type(param)=="string" then
        param={key=param}
    end
    local function menu_grabmenu(m, s, menu)
        return mod_menu.do_grabmenu(m, s, menu, param)
    end
    return menu_(mplex, sub, menu_or_name, menu_grabmenu, true)
end

--DOC
-- This function is similar to \fnref{mod_menu.bigmenu}, but input
-- is grabbed and \var{key} is used to cycle through the menu.
function mod_menu.biggrabmenu(mplex, sub, menu_or_name, key, initial) 
    local function menu_biggrabmenu(m, s, menu)
        return mod_menu.do_grabmenu(m, s, menu, true, key, initial or 0)
    end
    return menu_(mplex, sub, menu_or_name, menu_biggrabmenu, true, initial)
end

--DOC
-- This function displays a drop-down menu and should only
-- be called from a mouse press handler. The parameters are
-- similar to those of \fnref{mod_menu.menu}.
function mod_menu.pmenu(win, sub, menu_or_name) 
    return menu_(win, sub, menu_or_name, mod_menu.do_pmenu)
end

-- }}}


-- Workspace and window lists {{{

local function makelist(list)
    local function mkentry(tgt)
        return menuentry(tgt:name(), function() tgt:goto() end)
    end
    local entries=table.map(mkentry, list)
    table.sort(entries, function(a, b) return a.name < b.name end)
    return entries
end

function menus.windowlist()
    return makelist(ioncore.clientwin_list())
end

function menus.workspacelist()
    return makelist(ioncore.region_list("WGenWS"))
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
    dopath(look)

    local fname=ioncore.get_savefile('look')

    local function writeit()
        local f, err=io.open(fname, 'w')
        if not f then
            mod_query.message(where, err)
        else
            f:write(string.format('dopath("%s")\n', look))
            f:close()
        end
    end

    if not mod_query then
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
        query_message(where, TR("Cannot save selection."))
        return
    end
    
    mod_query.query_yesno(where, TR("Save look selection in %s?", fname),
                          writeit)
end

local function receive_styles(str)
    local data=""
    
    while str do
        data=data .. str
        if string.len(data)>ioncore.RESULT_DATA_LIMIT then
            error(TR("Too much result data"))
        end
        str=coroutine.yield()
    end
    
    local found={}
    local styles={}
    local stylemenu={}
    
    for look in string.gfind(data, "(look[-_][^\n]*)%.lua\n") do
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
    
    table.insert(stylemenu, menuentry(TR("Refresh list"),
                                      mod_menu.refresh_styles))
    
    menus.stylemenu=stylemenu
end


--DOC
-- Refresh list of known style files.
function mod_menu.refresh_styles()
    local cmd=ioncore.lookup_script("ion-completefile")
    if cmd then
        local path=ioncore.get_paths().searchpath
        local function mkarg(s)
            if s=="" then
                return ""
            else
                return (" "..string.shell_safe(s).."/look_"..
                        " "..string.shell_safe(s).."/look-")
            end
        end

        cmd=cmd..string.gsub(path..":", "([^:]*):", mkarg)
        
        ioncore.popen_bgread(cmd, coroutine.wrap(receive_styles))
    end
end

-- }}}


-- Context menu {{{


local function classes(reg)
    local function classes_(t)
        if t.__parentclass then
            classes_(t.__parentclass)
        end
        coroutine.yield(t.__typename)
    end
    return coroutine.wrap(function() classes_(reg) end)
end


local function get_ctxmenu(reg, sub, is_par)    
    local m={}
    
    local function cp(m2)
        local m3={}
        for k, v in m2 do
            local v2=table.copy(v)
            
            if v2.func then
                local ofunc=v2.func
                v2.func=function() return ofunc(reg, sub) end
            end
            
            if v2.submenu_fn then
                local ofn=v2.submenu_fn
                v2.submenu_fn=function() return cp(ofn()) end
            end
            
            m3[k]=v2
        end
        return m3
    end
    
    for s in classes(reg) do
        local m2=mod_menu.evalmenu("ctxmenu-"..s)
        if m2 then
            if is_par then
                m=table.icat(m, cp(m2))
            else
                m=table.icat(m, m2)
            end
        end
    end
    return m
end

function menus.ctxmenu(reg, sub)
    local m=get_ctxmenu(reg, sub, false);
    
    sub=reg
    reg=reg:manager()
    
    while reg do
        local mm = get_ctxmenu(reg, sub, true)
        if table.getn(mm)>0 then
            table.insert(m, mod_menu.submenu(reg:name(), mm))
        end
        sub=reg
        reg=reg:manager()
    end
    
    return m
end

-- }}}
export(mod_menu,
       "defmenu",
       "defctxmenu",
       "menuentry",
       "submenu")

mod_menu.refresh_styles()

-- Mark ourselves loaded.
_LOADED["mod_menu"]=true


-- Load configuration file
dopath('cfg_menu', true)
