--
-- ion/query/mod_query.lua -- Some common queries for Ion
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
if _LOADED["mod_query"] then return end

if not ioncore.load_module("mod_query") then
    return
end

local mod_query=_G["mod_query"]

assert(mod_query)


-- Generic helper functions {{{


function mod_query.make_completor(completefn)
    local function completor(wedln, str)
        wedln:set_completions(completefn(str))
    end
    return completor
end

    
function mod_query.query(mplex, prompt, initvalue, handler, completor)
    local function handle_it(str)
        handler(mplex, str)
    end
    -- Check that no other queries are open in the mplex.
    local l=mplex:llist(2)
    for i, r in l do
        if obj_is(r, "WEdln") then
            return
        end
    end
    mod_query.do_query(mplex, prompt, initvalue, handle_it, completor)
end


--DOC
-- This function query will display a query with prompt \var{prompt} in
-- \var{mplex} and if the user answers affirmately, call \var{handler}
-- with \var{mplex} as parameter.
function mod_query.query_yesno(mplex, prompt, handler)
    local function handler_yesno(mplex, str)
        if str=="y" or str=="Y" or str=="yes" then
            handler(mplex)
        end
    end
    return mod_query.query(mplex, prompt, nil, handler_yesno, nil)
end


function mod_query.query_execfile(mplex, prompt, prog)
    assert(prog~=nil)
    local function handle_execwith(mplex, str)
        ioncore.exec(prog.." "..string.shell_safe(str))
    end
    return mod_query.query(mplex, prompt, mod_query.get_initdir(),
                           handle_execwith, mod_query.file_completor)
end


function mod_query.query_execwith(mplex, prompt, dflt, prog, completor)
    local function handle_execwith(frame, str)
        if not str or str=="" then
            str=dflt
        end
        ioncore.exec(prog.." "..string.shell_safe(str))
    end
    return mod_query.query(mplex, prompt, nil, handle_execwith, completor)
end


function mod_query.lookup_script_warn(mplex, script)
    local script=ioncore.lookup_script(script)
    if not script then
        mod_query.warn(mplex, "Could not find "..script)
    end
    return script
end


function mod_query.get_initdir()
    if mod_query.last_dir then
        return mod_query.last_dir
    end
    local wd=os.getenv("PWD")
    if wd==nil then
        wd="/"
    elseif string.sub(wd, -1)~="/" then
        wd=wd .. "/"
    end
    return wd
end

local MAXDEPTH=10

function mod_query.lookup_workspace_classes()
    local classes={}
    
    for k, v in _G do
        if type(v)=="table" and v.__typename then
            v2=v.__parentclass
            for i=1, MAXDEPTH do
                if not v2 then 
                    break
                end
                if v2.__typename=="WGenWS" then
                    table.insert(classes, v.__typename)
                    break
                end
                v2=v2.__parentclass
            end
        end
    end
    
    return classes
end


function mod_query.complete_from_list(list, str)
    local results={}
    local len=string.len(str)
    if len==0 then
        results=list
    else
        for _, m in list do
            if string.sub(m, 1, len)==str then
                table.insert(results, m)
            end
        end
    end
    
    return results
end    


local pipes={}

mod_query.COLLECT_THRESHOLD=2000

--DOC
-- This function can be used to read completions from an external source.
-- The string \var{cmd} is a shell command to be executed. To its  stdout, 
-- the command should on the first line write the \var{common_part} 
-- parameter of \fnref{WEdln.set_completions} and a single actual completion
-- on each of the successive lines.
function mod_query.popen_completions(wedln, cmd, beg)
    
    local pst={wedln=wedln, maybe_stalled=0}
    
    local function rcv(str)
        local data=""
        local results={}
        local totallen=0
        local lines=0
        
        while str do
            if pst.maybe_stalled>=2 then
                pipes[rcv]=nil
                return
            end
            pst.maybe_stalled=0
            
            totallen=totallen+string.len(str)
            if totallen>ioncore.RESULT_DATA_LIMIT then
                error("Too much result data")
            end

            data=string.gsub(data..str, "([^\n]*)\n",
                             function(a)
                                 -- ion-completefile will return possible 
                                 -- common part of path on  the first line 
                                 -- and the entries in that directory on the
                                 -- following lines.
                                 if not results.common_part then
                                     results.common_part=(beg or "")..a
                                 else
                                     table.insert(results, a)
                                 end
                                 lines=lines+1
                             end)
            
            if lines>mod_query.COLLECT_THRESHOLD then
                collectgarbage()
                lines=0
            end
            
            str=coroutine.yield()
        end
        
        if not results.common_part then
            results.common_part=beg
        end
        
        wedln:set_completions(results)
        
        pipes[rcv]=nil
        results={}
        
        collectgarbage()
    end
    
    local found_clean=false
    
    for k, v in pipes do
        if v.wedln==wedln then
            if v.maybe_stalled<2 then
                v.maybe_stalled=v.maybe_stalled+1
                found_clean=true
            end
        end
    end
    
    if not found_clean then
        pipes[rcv]=pst
        ioncore.popen_bgread(cmd, coroutine.wrap(rcv))
    end
end


-- }}}


-- Simple queries for internal actions {{{


local function complete_name(str, list)
    local entries={}
    local l=string.len(str)
    for i, reg in list do
        local nm=reg:name()
        if nm and string.sub(nm, 1, l)==str then
            table.insert(entries, nm)
        end
    end
    if table.getn(entries)==0 then
        for i, reg in list do
            local nm=reg:name()
            if nm and string.find(nm, str, 1, true) then
                table.insert(entries, nm)
            end
        end
    end
    return entries
end

function mod_query.complete_clientwin(str)
    return complete_name(str, ioncore.clientwin_list())
end

function mod_query.complete_workspace(str)
    return complete_name(str, ioncore.region_list("WGenWS"))
end

function mod_query.complete_region(str)
    return complete_name(str, ioncore.region_list())
end


function mod_query.gotoclient_handler(frame, str)
    local cwin=ioncore.lookup_clientwin(str)
    
    if cwin==nil then
        mod_query.warn(frame, string.format('Could not find client window '..
                                           'named "%s".', str))
    else
        cwin:goto()
    end
end

function mod_query.attachclient_handler(frame, str)
    local cwin=ioncore.lookup_clientwin(str)
    
    if not cwin then
        mod_query.warn(frame, string.format('Could not find client window '..
                                            'named "%s".', str))
    elseif frame:rootwin_of()~=cwin:rootwin_of() then
        mod_query.warn(frame, "Cannot attach: not on same root window.")
    else
        frame:attach(cwin, { switchto = true })
    end
end


function mod_query.workspace_handler(mplex, name)
    local ws=ioncore.lookup_region(name, "WGenWS")
    if ws then
        ws:goto()
        return
    end
    
    local classes=mod_query.lookup_workspace_classes()
    
    local function completor(wedln, what)
        local results=mod_query.complete_from_list(classes, what)
        wedln:set_completions(results)
    end
    
    local function handler(mplex, cls)
        local scr=mplex:screen_of()
        if not scr then
            mod_query.warn(mplex, "Unable to create workspace: no screen.")
            return
        end
        
        if not cls or cls=="" then
            cls=ioncore.get().default_ws_type
        end
        
        local err=collect_errors(function()
                                     ws=scr:attach_new({ 
                                         type=cls, 
                                         name=name, 
                                         switchto=true 
                                     })
                                 end)
        if not ws then
            mod_query.warn(mplex, err or "Unknown error")
        end
    end
    
    local defcls=ioncore.get().default_ws_type
    local prompt="Workspace type"
    if defcls then
        prompt=prompt.."("..defcls..")"
    end
    prompt=prompt..":"
    
    mod_query.query(mplex, prompt, "", handler, completor)
end


--DOC
-- This query asks for the name of a client window and attaches
-- it to the frame the query was opened in. It uses the completion
-- function \fnref{ioncore.complete_clientwin}.
function mod_query.query_gotoclient(mplex)
    mod_query.query(mplex, "Go to window:", nil,
                    mod_query.gotoclient_handler,
                    mod_query.make_completor(mod_query.complete_clientwin))
end

--DOC
-- This query asks for the name of a client window and switches
-- focus to the one entered. It uses the completion function
-- \fnref{ioncore.complete_clientwin}.
function mod_query.query_attachclient(mplex)
    mod_query.query(mplex, "Attach window:", nil,
                    mod_query.attachclient_handler, 
                    mod_query.make_completor(mod_query.complete_clientwin))
end


--DOC
-- This query asks for the name of a workspace. If a workspace
-- (an object inheriting \type{WGenWS}) with such a name exists,
-- it will be switched to. Otherwise a new workspace with the
-- entered name will be created and the user will be queried for
-- the type of the workspace.
function mod_query.query_workspace(mplex)
    mod_query.query(mplex, "Go to or create workspace:", nil, 
                    mod_query.workspace_handler,
                    mod_query.make_completor(mod_query.complete_workspace))
end


--DOC
-- This query asks whether the user wants to exit Ion (no session manager)
-- or close the session (running under a session manager that supports such
-- requests). If the answer is 'y', 'Y' or 'yes', so will happen.
function mod_query.query_shutdown(mplex)
    mod_query.query_yesno(mplex, "Exit Ion/Shutdown session (y/n)?", 
                         ioncore.shutdown)
end


--DOC
-- This query asks whether the user wants restart Ioncore.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
function mod_query.query_restart(mplex)
    mod_query.query_yesno(mplex, "Restart Ion (y/n)?", ioncore.restart)
end


--DOC
-- This function asks for a name new for the frame where the query
-- was created.
function mod_query.query_renameframe(frame)
    mod_query.query(frame, "Frame name:", frame:name(),
                    function(frame, str) frame:set_name(str) end,
                    nil)
end


--DOC
-- This function asks for a name new for the workspace on which the
-- query resides.
function mod_query.query_renameworkspace(mplex)
    local ws=ioncore.find_manager(mplex, "WGenWS")
    mod_query.query(mplex, "Workspace name:", ws:name(),
                    function(mplex, str) ws:set_name(str) end,
                    nil)
end


-- }}}


-- Run/view/edit {{{


function mod_query.file_completor(wedln, str, wp, beg)
    local ic=ioncore.lookup_script("ion-completefile")
    if ic then
        mod_query.popen_completions(wedln,
                                   ic..(wp or " ")..string.shell_safe(str),
                                   beg)
    end
end


--DOC
-- Asks for a file to be edited. It uses the script \file{ion-edit} to
-- start a program to edit the file. This script uses \file{run-mailcap}
-- by default, but if you don't have it, you may customise the script.
function mod_query.query_editfile(mplex)
    local script=mod_query.lookup_script_warn(mplex, "ion-edit")
    mod_query.query_execfile(mplex, "Edit file:", script)
end


--DOC
-- Asks for a file to be viewed. It uses the script \file{ion-view} to
-- start a program to view the file. This script uses \file{run-mailcap}
-- by default, but if you don't have it, you may customise the script.
function mod_query.query_runfile(mplex)
    local script=mod_query.lookup_script_warn(mplex, "ion-view")
    mod_query.query_execfile(mplex, "View file:", script)
end


function mod_query.exec_completor(wedln, str)
    local st, en, beg, tocompl=string.find(str, "^(.-)([^%s]*)$")
    if string.len(beg)==0 then
        mod_query.file_completor(wedln, tocompl, " -wp ")
    else
        mod_query.file_completor(wedln, tocompl, " ", beg)
    end
end


function mod_query.exec_handler(frame, cmd)
    if string.sub(cmd, 1, 1)==":" then
        local ix=mod_query.lookup_script_warn(frame, "ion-runinxterm")
        if not ix then return end
        cmd=ix.." "..string.sub(cmd, 2)
    end
    ioncore.exec_on(frame:rootwin_of(), cmd)
end

--DOC
-- This function asks for a command to execute with \file{/bin/sh}.
-- If the command is prefixed with a colon (':'), the command will
-- be run in an XTerm (or other terminal emulator) using the script
-- \file{ion-runinxterm}.
function mod_query.query_exec(mplex)
    mod_query.query(mplex, "Run:", nil, mod_query.exec_handler, 
                    mod_query.exec_completor)
end


-- }}}


-- SSH {{{


mod_query.known_hosts={}


function mod_query.get_known_hosts(mplex)
    mod_query.known_hosts={}
    local f
    local h=os.getenv("HOME")
    if h then 
        f=io.open(h.."/.ssh/known_hosts")
    end
    if not f then 
        warn("Failed to open ~/.ssh/known_hosts") 
        return
    end
    for l in f:lines() do
        local st, en, hostname=string.find(l, "^([^%s,]+)")
        if hostname then
            table.insert(mod_query.known_hosts, hostname)
        end
    end
    f:close()
end


function mod_query.complete_ssh(str)
    local st, en, user, at, host=string.find(str, "^([^@]*)(@?)(.*)$")
    
    if string.len(at)==0 and string.len(host)==0 then
        host = user; user = ""
    end
    
    if at=="@" then 
        user = user .. at 
    end
    
    local res = {}
    
    if string.len(host)==0 then
        if string.len(user)==0 then
            return mod_query.known_hosts
        end
        
        for _, v in ipairs(mod_query.known_hosts) do
            table.insert(res, user .. v)
        end
        return res
    end
    
    for _, v in ipairs(mod_query.known_hosts) do
        local s, e=string.find(v, host, 1, true)
        if s==1 and e>=1 then
            table.insert(res, user .. v)
        end
    end
    
    return res
end


--DOC
-- This query asks for a host to connect to with SSH. It starts
-- up ssh in a terminal using \file{ion-ssh}. Hosts to tab-complete
-- are read from \file{\~{}/.ssh/known\_hosts}.
function mod_query.query_ssh(mplex)
    mod_query.get_known_hosts(mplex)
    local script=mod_query.lookup_script_warn(mplex, "ion-ssh")
    mod_query.query_execwith(mplex, "SSH to:", nil, script,
                             mod_query.make_completor(mod_query.complete_ssh))
end


-- }}}


-- Man pages {{{{


function mod_query.man_completor(wedln, str)
    local mc=ioncore.lookup_script("ion-completeman")
    if mc then
        mod_query.popen_completions(wedln, mc.." -complete "..
                                    string.shell_safe(str))
    end
end


--DOC
-- This query asks for a manual page to display. It uses the command
-- \file{ion-man} to run \file{man} in a terminal emulator. By customizing
-- this script it is possible use some other man page viewer. The script
-- \file{ion-completeman} is used to complete manual pages.
function mod_query.query_man(mplex)
    local script=mod_query.lookup_script_warn(mplex, "ion-man")
    mod_query.query_execwith(mplex, "Manual page (ion):", "ion",
                             script, mod_query.man_completor)
end


-- }}}


-- Lua code execution {{{


function mod_query.create_run_env(mplex)
    local origenv=getfenv()
    local meta={__index=origenv, __newindex=origenv}
    local env={
        _=mplex, 
        _sub=mplex:current(),
    }
    setmetatable(env, meta)
    return env
end

function mod_query.do_handle_lua(mplex, env, code)
    local f, err=loadstring(code)
    if not f then
        mod_query.warn(mplex, err)
        return
    end
    setfenv(f, env)
    err=collect_errors(f)
    if err then
        mod_query.warn(mplex, err)
    end
end

local function getindex(t)
    local mt=getmetatable(t)
    if mt then return mt.__index end
    return nil
end

function mod_query.do_complete_lua(env, str)
    -- Get the variable to complete, including containing tables.
    -- This will also match string concatenations and such because
    -- Lua's regexps don't support optional subexpressions, but we
    -- handle them in the next step.
    local comptab=env
    local metas=true
    local _, _, tocomp=string.find(str, "([%w_.:]*)$")
    
    -- Descend into tables
    if tocomp and string.len(tocomp)>=1 then
        for t in string.gfind(tocomp, "([^.:]*)[.:]") do
            metas=false
            if string.len(t)==0 then
                comptab=env;
            elseif comptab then
                if type(comptab[t])=="table" then
                    comptab=comptab[t]
                elseif type(comptab[t])=="userdata" then
                    comptab=getindex(comptab[t])
                    metas=true
                else
                    comptab=nil
                end
            end
        end
    end
    
    if not comptab then return {} end
    
    local compl={}
    
    -- Get the actual variable to complete without containing tables
    _, _, compl.common_part, tocomp=string.find(str, "(.-)([%w_]*)$")
    
    local l=string.len(tocomp)
    
    local tab=comptab
    local seen={}
    while true do
        for k in tab do
            if type(k)=="string" then
                if string.sub(k, 1, l)==tocomp then
                    table.insert(compl, k)
                end
            end
        end
        
        -- We only want to display full list of functions for objects, not 
        -- the tables representing the classes.
        --if not metas then break end
        
        seen[tab]=true
        tab=getindex(tab)
        if not tab or seen[tab] then break end
    end
    
    -- If there was only one completion and it is a string or function,
    -- concatenate it with "." or "(", respectively.
    if table.getn(compl)==1 then
        if type(comptab[compl[1]])=="table" then
            compl[1]=compl[1] .. "."
        elseif type(comptab[compl[1]])=="function" then
            compl[1]=compl[1] .. "("
        end
    end
    
    return compl
end


--DOC
-- This query asks for Lua code to execute. It sets the variable '\var{\_}'
-- in the local environment of the string to point to the mplex where the
-- query was created. It also sets the table \var{arg} in the local
-- environment to \code{\{_, _:current()\}}.
function mod_query.query_lua(mplex)
    local env=mod_query.create_run_env(mplex)
    
    local function complete(wedln, code)
        wedln:set_completions(mod_query.do_complete_lua(env, code))
    end
    
    local function handler(mplex, code)
        return mod_query.do_handle_lua(mplex, env, code)
    end
    
    mod_query.query(mplex, "Lua code to run: ", nil, handler, complete)
end

-- }}}


-- Menu query {{{

--DOC
-- This query can be used to create a query of a defined menu.
function mod_query.query_menu(mplex, prompt, menuname)
    if not mod_menu then
        mod_query.warn(mplex, "mod_menu not loaded.")
        return
    end
    
    local menu=mod_menu.getmenu(menuname)
    
    if not menu then
        mod_query.warn(mplex, "Unknown menu "..tostring(menuname))
        return
    end
    function complete(str)
        local results={}
        local len=string.len(str)
        for _, m in menu do
            local mn=m.name
            if len==0 or string.sub(mn, 1, len)==str then
                table.insert(results, mn)
            end
        end
        return results
    end    
    
    local function handle(mplex, str)
        local e
        for k, v in menu do
            if v.name==str then
                e=v
                break
            end
        end
        if e then
            if e.func then
                local err=collect_errors(function() e.func(mplex) end)
                if err then
                    mod_query.warn(mplex, err)
                end
            elseif e.submenu_fn then
                mod_query.query_menu(mplex, e.name.." menu:", e.submenu_fn())
            end
        else
            mod_query.warn(mplex, "No entry '"..str.."'.")
        end
    end
    
    mod_query.query(mplex, prompt, nil, handle, 
                    mod_query.make_completor(complete))
end

-- }}}


-- Miscellaneous {{{


--DOC 
-- Display an "About Ion" message in \var{mplex}.
function mod_query.show_about_ion(mplex)
    mod_query.message(mplex, ioncore.aboutmsg())
end


--DOC
-- Show information about a client window.
function mod_query.show_clientwin(mplex, cwin)
    local function indent(s)
        local i="    "
        return i..string.gsub(s, "\n", "\n"..i)
    end
    
    local function get_info(cwin)
        local function n(s) return (s or "") end
        local f="Title: %s\nClass: %s\nRole: %s\nInstance: %s\nXID: 0x%x"
        local i=cwin:get_ident()
        local s=string.format(f, n(cwin:name()), n(i.class), n(i.role), 
                              n(i.instance), cwin:xid())
        local t="\nTransients:\n"
        for k, v in cwin:managed_list() do
            if obj_is(v, "WClientWin") then
                s=s..t..indent(get_info(v))
                t="\n"
            end
        end
        return s
    end
    
    mod_query.message(mplex, get_info(cwin))
end

-- }}}


-- Mark ourselves loaded.
_LOADED["mod_query"]=true
