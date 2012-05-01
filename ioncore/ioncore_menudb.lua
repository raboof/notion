--
-- ion/ioncore/ioncore_menudb.lua -- Routines for defining menus.
-- 
-- Copyright (c) Tuomo Valkonen 2004-2009.
--
-- See the included file LICENSE for details.
--

local ioncore=_G.ioncore


-- Table to hold defined menus.
local menus={}


-- Menu construction {{{

--DOC
-- Define a new menu with \var{name} being the menu's name and \var{tab} 
-- being a table of menu entries. If \var{tab.append} is set, the entries 
-- are appended to previously-defined ones, if possible.
function ioncore.defmenu(name, tab)
    if menus[name] and type(tab)=="table" and tab.append then
        if type(menus[name])~="table" then
            ioncore.warn(TR("Unable to append to non-table menu"))
            return
        else
            for k, v in ipairs(tab) do
                table.insert(menus[name], v)
            end
        end
    else
        menus[name]=tab
    end
end

--DOC
-- Returns a menu defined with \fnref{ioncore.defmenu}.
function ioncore.getmenu(name)
    return menus[name]
end

--DOC
-- Define context menu for context \var{ctx}, \var{tab} being a table 
-- of menu entries.
function ioncore.defctxmenu(ctx, ...)
    local tab, add
    local a1, a2 = ...
    if a2 and type(a1)=="string" then
        tab=a2
        tab.label=ioncore.gettext(a1)
    else
        tab=a1
    end
    ioncore.defmenu("ctxmenu-"..ctx, tab)
end

--DOC
-- Returns a context menu defined with \fnref{ioncore.defctxmenu}.
function ioncore.getctxmenu(name)
    return menus["ctxmenu-"..name]
end

function ioncore.evalmenu(menu, ...)
    if type(menu)=="string" then
        return ioncore.evalmenu(menus[menu], ...)
    elseif type(menu)=="function" then
        return menu(...)
    elseif type(menu)=="table" then
        return menu
    end
end

--DOC
-- Use this function to define normal menu entries. The string \var{name} 
-- is the string shown in the visual representation of menu. The
-- parameter \var{cmd} and \var{guard_or_opts} (when string) are similar
-- to those of \fnref{ioncore.defbindings}.  If \var{guard_or_opts} is
-- a table, it may contains the \var{guard} field, and the \var{priority}
-- field, for controlling positioning of entries in context menus.
-- (The default priority is 1 for most entries, and -1 for auto-generated
-- submenus.)
function ioncore.menuentry(name, cmd, guard_or_opts)
    local guard
    local opts
    
    if type(guard_or_opts)=="string" then
        guard=guard_or_opts
    elseif type(guard_or_opts)=="table" then
        opts=guard_or_opts
        guard=opts.guard
    end
    
    local fn, gfn=ioncore.compile_cmd(cmd, guard)
    if fn then
        return table.append({
                   name=ioncore.gettext(name), 
                   func=fn, 
                   guard_func=gfn, 
               }, opts or {})
    end
end


--DOC
-- Use this function to define menu entries for submenus. The parameter
-- \fnref{sub_or_name} is either a table of menu entries or the name
-- of an already defined menu. The initial menu entry to highlight can be
-- specified by \var{options.initial} as either an integer starting from 1, 
-- or a  function that returns such a number. Another option supported is
-- \var{options.noautoexpand} that will cause \fnref{mod_query.query_menu}
-- to not automatically expand this submenu.
function ioncore.submenu(name, sub_or_name, options)
    return table.append({
               name=ioncore.gettext(name),
               submenu_fn=function()
                              return ioncore.evalmenu  (sub_or_name)
                          end,
           }, options or {})
end


-- }}}


-- Workspace and window lists {{{

local function addto(list)
    return function(tgt, attr)
        local e=menuentry(tgt:name(), function() tgt:display() end)
        e.attr=attr;
        table.insert(list, e)
        return true
    end
end
    
local function sort(entries)
    table.sort(entries, function(a, b) return a.name < b.name end)
    return entries
end

function menus.windowlist()
    local entries={}
    ioncore.clientwin_i(addto(entries))
    return sort(entries)
end

function menus.workspacelist()
    local entries={}
    local iter_=addto(entries)
    
    local function iter(obj) 
        return (not obj_is(obj, "WGroupWS") 
                or iter_(obj))
    end
    
    ioncore.region_i(iter)
    
    return sort(entries)
end

local function focuslist(do_act)
    local entries={}
    local seen={}
    local iter_=addto(entries)
    
    local function iter(obj, attr) 
        if obj_is(obj, "WClientWin") then
            iter_(obj, attr)
            seen[obj]=true
        end
        return true
    end
    
    local function iter_act(obj)
        return iter(obj, "activity")
    end
    
    local function iter_foc(obj)
        return (seen[obj] or iter(obj))
    end
    
    if do_act then
        -- Windows with activity first
        ioncore.activity_i(iter_act)
    end
    
    -- The ones that have been focused in their lifetime
    ioncore.focushistory_i(iter_foc)
    
    -- And then the rest
    ioncore.clientwin_i(iter_foc)
    
    return entries
end

menus.focuslist=function() return focuslist(true) end
menus.focuslist_=function() return focuslist(false) end

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
    
    for look in string.gmatch(data, "(look[-_][^\n]*)%.lua\n") do
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
                                      ioncore.refresh_stylelist))
    
    menus.stylemenu=stylemenu
end


--DOC
-- Refresh list of known style files.
function ioncore.refresh_stylelist()
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
            return ""
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


local function modeparts(mode)
    if not mode then
        return function() return end
    end
    
    local f, s, v=string.gmatch(mode, "(%-?[^-]+)");
    
    local function nxt(_, m)
        v = f(s, v)
        return (v and (m .. v))
    end
    
    return nxt, nil, ""
end


local function get_ctxmenu(reg, sub)
    local m={}
    
    local function cp(m2)
        local m3={}
        for k, v in ipairs(m2) do
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
        m3.label=m2.label
        return m3
    end
    
    local function add_ctxmenu(m2, use_label)
        if m2 then
            m=table.icat(m, cp(m2))
            m.label=(use_label and m2.label) or m.label
        end
    end
    
    local mgr=reg:manager()
    local mgrname=(mgr and mgr:name()) or nil
    local mode=(reg.mode and reg:mode())

    for s in classes(reg) do
        local nm="ctxmenu-"..s
        add_ctxmenu(ioncore.evalmenu(nm), true)
        for m in modeparts(mode) do
            add_ctxmenu(ioncore.evalmenu(nm.."."..m), false)
        end
        if mgrname then
            add_ctxmenu(ioncore.evalmenu(nm.."@"..mgrname), false)
        end
    end
    return m
end


local function sortmenu(m)
    local v=1/2
    
    for _, e in ipairs(m) do
        e.priority=(e.priority or 1)+v
        v=v/2
    end
    
    table.sort(m, function(e1, e2) return e1.priority > e2.priority end)
    
    return m
end


function menus.ctxmenu(reg, sub)
    local m, r, s
    
    if obj_is(sub, "WGroup") then
        sub=(sub:bottom() or sub)
    end
    
    -- First, stuff between reg (inclusive) and sub_or_chld (inclusive)
    -- at the top level in the menu.
    r=(sub or reg)
    while r and s~=reg do
        local mm=get_ctxmenu(r, s)
        m=((m and table.icat(mm, m)) or mm)
        s=r
        r=r:manager()
    end
    
    m=(m or {})
    
    -- Then stuff below reg (exclusive) as submenus
    while r do
        local mm = get_ctxmenu(r, s)
        if #mm>0 then
            local nm=mm.label or obj_typename(reg)
            local tmp=ioncore.submenu(nm, sortmenu(mm), {priority=-1})
            table.insert(m, tmp)
        end
        s=r
        r=r:manager()
    end
    
    return sortmenu(m)
end

-- }}}

ioncore.refresh_stylelist()

