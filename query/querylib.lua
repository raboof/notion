--
-- ion/query/querylib.lua -- Some common queries for Ion
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
if _LOADED["querylib"] then return end

local querylib={}
_G.querylib=querylib

-- Generic helper functions {{{


function querylib.make_completor(completefn)
    local function completor(wedln, str)
        wedln:set_completions(completefn(str))
    end
    return completor
end

    
function querylib.do_query(mplex, prompt, initvalue, handler, completor)
    local function handle_it(str)
        handler(mplex, str)
    end
    -- Check that no other queries are open in the mplex.
    local l=mplex:l2_list()
    for i, r in l do
        if obj_is(r, "WEdln") then
            return
        end
    end
    mod_query.query(mplex, prompt, initvalue, handle_it, completor)
end


--DOC
-- This function query will display a query with prompt \var{prompt} in
-- \var{mplex} and if the user answers affirmately, call \var{handler}
-- with \var{mplex} as parameter.
function querylib.query_yesno(mplex, prompt, handler)
    local function handler_yesno(mplex, str)
        if str=="y" or str=="Y" or str=="yes" then
            handler(mplex)
        end
    end
    return querylib.do_query(mplex, prompt, nil, handler_yesno, nil)
end


function querylib.do_query_execfile(mplex, prompt, prog)
    assert(prog~=nil)
    local function handle_execwith(mplex, str)
        ioncore.exec(prog.." "..string.shell_safe(str))
    end
    return querylib.do_query(mplex, prompt, querylib.get_initdir(),
                             handle_execwith, querylib.file_completor)
end


function querylib.do_query_execwith(mplex, prompt, dflt, prog, completor)
    local function handle_execwith(frame, str)
        if not str or str=="" then
            str=dflt
        end
        ioncore.exec(prog.." "..string.shell_safe(str))
    end
    return querylib.do_query(mplex, prompt, nil, 
                             handle_execwith, completor)
end


function querylib.lookup_script_warn(mplex, script)
    local script=ioncore.lookup_script(script)
    if not script then
        mod_query.warn(mplex, "Could not find "..script)
    end
    return script
end


function querylib.get_initdir()
    if querylib.last_dir then
        return querylib.last_dir
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

function querylib.lookup_workspace_classes()
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


function querylib.complete_from_list(list, str)
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

querylib.COLLECT_THRESHOLD=2000

--DOC
-- This function can be used to read completions from an external source.
-- The string \var{cmd} is a shell command to be executed. To its  stdout, 
-- the command should on the first line write the \var{common_part} 
-- parameter of \fnref{WEdln.set_completions} and a single actual completion
-- on each of the successive lines.
function querylib.popen_completions(wedln, cmd)
    
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
            if totallen>ioncorelib.RESULT_DATA_LIMIT then
                error("Too much result data")
            end

            data=string.gsub(data..str, "([^\n]*)\n",
                             function(a)
                                 -- ion-completefile will return possible 
                                 -- common part of path on  the first line 
                                 -- and the entries in that directory on the
                                 -- following lines.
                                 if not results.common_part then
                                     results.common_part=a
                                 else
                                     table.insert(results, a)
                                 end
                                 lines=lines+1
                             end)
            
            if lines>querylib.COLLECT_THRESHOLD then
                collectgarbage()
                lines=0
            end
            
            str=coroutine.yield()
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

function querylib.complete_clientwin(str)
    return complete_name(str, ioncore.clientwin_list())
end

function querylib.complete_workspace(str)
    return complete_name(str, ioncore.region_list("WGenWS"))
end

function querylib.complete_region(str)
    return complete_name(str, ioncore.region_list())
end


function querylib.gotoclient_handler(frame, str)
    local cwin=ioncore.lookup_clientwin(str)
    
    if cwin==nil then
        mod_query.warn(frame, string.format('Could not find client window '..
                                           'named "%s".', str))
    else
        cwin:goto()
    end
end

function querylib.attachclient_handler(frame, str)
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


function querylib.workspace_handler(mplex, name)
    local ws=ioncore.lookup_region(name, "WGenWS")
    if ws then
        ws:goto()
        return
    end
    
    local classes=querylib.lookup_workspace_classes()
    
    local function completor(wedln, what)
        local results=querylib.complete_from_list(classes, what)
        wedln:set_completions(results)
    end
    
    local function handler(mplex, cls)
        local scr=mplex:screen_of()
        if not scr then
            mod_query.warn(mplex, "Unable to create workspace: no screen.")
            return
        end
        
        if not cls or cls=="" then
            cls=DEFAULT_WS_TYPE
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
    
    local prompt="Workspace type ("..DEFAULT_WS_TYPE.."):"
    
    querylib.do_query(mplex, prompt, "", handler, completor)
end


--DOC
-- This query asks for the name of a client window and attaches
-- it to the frame the query was opened in. It uses the completion
-- function \fnref{ioncore.complete_clientwin}.
function querylib.query_gotoclient(mplex)
    querylib.do_query(mplex, "Go to window:", nil,
                      querylib.gotoclient_handler,
                      querylib.make_completor(querylib.complete_clientwin))
end

--DOC
-- This query asks for the name of a client window and switches
-- focus to the one entered. It uses the completion function
-- \fnref{ioncore.complete_clientwin}.
function querylib.query_attachclient(mplex)
    querylib.do_query(mplex, "Attach window:", nil,
                      querylib.attachclient_handler, 
                      querylib.make_completor(querylib.complete_clientwin))
end


--DOC
-- This query asks for the name of a workspace. If a workspace
-- (an object inheriting \type{WGenWS}) with such a name exists,
-- it will be switched to. Otherwise a new workspace with the
-- entered name will be created and the user will be queried for
-- the type of the workspace.
function querylib.query_workspace(mplex)
    querylib.do_query(mplex, "Go to or create workspace:", nil, 
                      querylib.workspace_handler,
                      querylib.make_completor(querylib.complete_workspace))
end


--DOC
-- This query asks whether the user wants to have Ioncore exit.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
function querylib.query_exit(mplex)
    querylib.query_yesno("Exit Ion (y/n)?", exit_wm)
end


--DOC
-- This query asks whether the user wants restart Ioncore.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
function querylib.query_restart(mplex)
    querylib.query_yesno("Restart Ion (y/n)?", restart_wm)
end


--DOC
-- This function asks for a name new for the frame where the query
-- was created.
function querylib.query_renameframe(frame)
    querylib.do_query(frame, "Frame name:", frame:name(),
                      function(frame, str) frame:set_name(str) end,
                      nil)
end


--DOC
-- This function asks for a name new for the workspace on which the
-- query resides.
function querylib.query_renameworkspace(mplex)
    local ws=ioncorelib.find_manager(ws, "WGenWS")
    querylib.do_query(frame, "Workspace name:", ws:name(),
                      function(mplex, str) ws:set_name(str) end,
                      nil)
end


-- }}}


-- Run/view/edit {{{


function querylib.file_completor(wedln, str, wp)
    local ic=ioncore.lookup_script("ion-completefile")
    if ic then
        querylib.popen_completions(wedln,
                                   ic..(wp or " ")..string.shell_safe(str))
    end
end


--DOC
-- Asks for a file to be edited. It uses the script \file{ion-edit} to
-- start a program to edit the file. This script uses \file{run-mailcap}
-- by default, but if you don't have it, you may customise the script.
function querylib.query_editfile(mplex)
    local script=querylib.lookup_script_warn(mplex, "ion-edit")
    querylib.do_query_execfile(mplex, "Edit file:", script)
end


--DOC
-- Asks for a file to be viewed. It uses the script \file{ion-view} to
-- start a program to view the file. This script uses \file{run-mailcap}
-- by default, but if you don't have it, you may customise the script.
function querylib.query_runfile(mplex)
    local script=querylib.lookup_script_warn(mplex, "ion-view")
    querylib.do_query_execfile(mplex, "View file:", script)
end


function querylib.exec_completor(wedln, str)
    querylib.file_completor(wedln, str, " -wp ")
end

function querylib.exec_handler(frame, cmd)
    if string.sub(cmd, 1, 1)==":" then
        local ix=querylib.lookup_script_warn(frame, "ion-runinxterm")
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
function querylib.query_exec(mplex)
    querylib.do_query(mplex, "Run:", nil, 
                      querylib.exec_handler, querylib.exec_completor)
end


-- }}}


-- SSH {{{


querylib.known_hosts={}


function querylib.get_known_hosts(mplex)
    querylib.known_hosts={}
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
            table.insert(querylib.known_hosts, hostname)
        end
    end
    f:close()
end


function querylib.complete_ssh(str)
    if string.len(str)==0 then
        return querylib.known_hosts
    end
    
    local res={}
    for _, v in ipairs(querylib.known_hosts) do
        local s, e=string.find(v, str, 1, true)
        if s==1 and e>=1 then
            table.insert(res, v)
        end
    end
    return res
end


--DOC
-- This query asks for a host to connect to with SSH. It starts
-- up ssh in a terminal using \file{ion-ssh}. Hosts to tab-complete
-- are read from \file{\~{}/.ssh/known\_hosts}.
function querylib.query_ssh(mplex)
    querylib.get_known_hosts(mplex)
    local script=querylib.lookup_script_warn(mplex, "ion-ssh")
    querylib.do_query_execwith(mplex, "SSH to:", nil, script,
                               querylib.make_completor(querylib.complete_ssh))
end


-- }}}


-- Man pages {{{{


function querylib.man_completor(wedln, str)
    local mc=ioncore.lookup_script("ion-completeman")
    if mc then
        querylib.popen_completions(wedln, mc.." -complete "..
                                   string.shell_safe(str))
    end
end


--DOC
-- This query asks for a manual page to display. It uses the command
-- \file{ion-man} to run \file{man} in a terminal emulator. By customizing
-- this script it is possible use some other man page viewer. The script
-- \file{ion-completeman} is used to complete manual pages.
function querylib.query_man(mplex)
    local script=querylib.lookup_script_warn(mplex, "ion-man")
    querylib.do_query_execwith(mplex, "Manual page (ion):", "ion",
                               script, querylib.man_completor)
end


-- }}}


-- Lua code execution {{{


function querylib.create_run_env(mplex)
    local origenv=getfenv()
    local meta={__index=origenv, __newindex=origenv}
    local env={
        _=mplex, 
        _sub=mplex:current(),
    }
    setmetatable(env, meta)
    return env
end

function querylib.do_handle_lua(mplex, env, code)
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

function querylib.do_complete_lua(env, str)
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
function querylib.query_lua(mplex)
    local env=querylib.create_run_env(mplex)
    
    local function complete(wedln, code)
        wedln:set_completions(querylib.do_complete_lua(env, code))
    end
    
    local function handler(mplex, code)
        return querylib.do_handle_lua(mplex, env, code)
    end
    
    querylib.do_query(mplex, "Lua code to run: ", nil, handler, complete)
end

-- }}}


-- Miscellaneous {{{


--DOC 
-- Display an "About Ion" message in \var{mplex}.
function querylib.show_about_ion(mplex)
    mod_query.message(mplex, ioncore.aboutmsg())
end


-- }}}


-- Mark ourselves loaded.
_LOADED["querylib"]=true
