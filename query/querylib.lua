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


querylib={}

-- Functions to generate functions {{{


--DOC
-- Generate a ''completor'' passable to many of the \code{querylib.make_*_fn}
-- functions from a simple function \var{completefn} that completes an input 
-- string to a table of completion strings.
function querylib.make_completor(completefn)
    local function completor(wedln, str)
        wedln:set_completions(completefn(str))
    end
    return completor
end

    
--DOC
-- Generate a function that can be directly passed in mplex/frame/screen 
-- binding definitions to display a query to call a function with 
-- tab-completed parameter. The parameters to this function are
-- \begin{tabularx}{\linewidth}{lX}
-- \hline
-- Parameter & Description \\
-- \hline
-- \var{prompt} & The prompt. \\
-- \var{initfn} & A function that returns initial input when called. \\
-- \var{handler} & The function to be called with the user input as parameter 
-- 		   when the query is finished. \\
-- \var{completor} & A completor function. \\
-- \end{tabularx}
function querylib.make_simple_fn(prompt, initfn, handler, completor)
    local function query_it(frame)
        local initvalue;
        if initfn then
            initvalue=initfn()
        end
        query_query(frame, prompt, initvalue or "", handler, completor)
    end
    return query_it
end

--DOC
-- Generate a function that can be directly passed in mplex/frame/screen 
-- binding definitions to display a query to call a function with 
-- tab-completed parameter. The parameters to this function are
-- \begin{tabularx}{\linewidth}{lX}
-- \hline
-- Parameter & Description \\
-- \hline
-- \var{prompt} & The prompt. \\
-- \var{initfn} & A function that returns initial input when called. \\
-- \var{handler} & The function to be called with the mplex/frame/screen and
--                 user input as parameters when the query is finished. \\
-- \var{completor} & A completor function. \\
-- \end{tabularx}
function querylib.make_frame_fn(prompt, initfn, handler, completor)
    local function query_it(frame)
        local initvalue;
        if initfn then
            initvalue=initfn(frame)
        end
        local function handle_it(str)
            handler(frame, str)
        end
        query_query(frame, prompt, initvalue or "", handle_it, completor)
    end
    return query_it
end

function querylib.make_rename_fn(prompt, getobj)
    local function query_it(frame)
        local obj=getobj(frame)
        local function handle_it(str)
            obj:set_name(str)
        end
        query_query(frame, prompt, obj:name() or "", handle_it, nil)
    end
    return query_it
end

--DOC
-- Generate a function that can be directly passed in mplex/frame/screen 
-- binding definitions to display a query to conditionally depending 
-- on user's input call another function \var{handler} that will get the
-- mplex/frame/screen where the query was displayed as argument.
function querylib.make_yesno_fn(prompt, handler)
    local function handle_yesno(...)
        local n=table.getn(arg)
        if n==0 then return end
        if arg[n]=="y" or arg[n]=="Y" or arg[n]=="yes" then
            table.remove(arg, n)
            handler(unpack(arg))
        end
    end
    return querylib.make_frame_fn(prompt, nil, handle_yesno, nil)
end

function querylib.make_script_lookup_fn(script)
    return function()
               local s=lookup_script(script)
               if not s then
                   return nil, "Could not find "..script
               else
                   return s
               end
           end
end

local function get_script_warn(frame, script)
    local s, err=querylib.make_script_lookup_fn(script)()
    if not s then
        query_fwarn(frame, err)
    end
    return s
end
    
local function getprog(prog)
    if type(prog)=="function" then
        return prog()
    elseif type(prog)=="string" then
        return prog
    else
        return nil, "No program to run specified"
    end
end

--DOC
-- Generate a function that can be directly passed in mplex/frame/screen 
-- binding definitions to display a query to start a program with a 
-- tab-completed parameter. The parameters to this function are
-- \begin{tabularx}{\linewidth}{lX}
-- \hline
-- Parameter & Description \\
-- \hline
-- \var{prompt} & The prompt \\
-- \var{dflt} & Default value to call \var{prog} with \\
-- \var{prog} & Program name or a function that returns it when called. \\
-- \var{completor} & A completor function. \\
-- \end{tabularx}
function querylib.make_execwith_fn(prompt, dflt, prog, completor)
    local function handle_execwith(frame, str)
        local p, err=getprog(prog)
        if p then
            if not str or str=="" then
                str=dflt
            end
            exec_in(frame, p.." "..string.shell_safe(str))
        else
            query_fwarn(frame, err)
        end
    end
    return querylib.make_frame_fn(prompt, nil, handle_execwith, completor)
end

--DOC
-- Generate a function that can be directly passed in mplex/frame/screen 
-- binding definitions to display a query to start a program with a 
-- tab-completed file as parameter. The previously inputted file's directory is 
-- used as the initial input. The parameters to this function are
-- \begin{tabularx}{\linewidth}{lX}
-- \hline
-- Parameter & Description \\
-- \hline
-- \var{prompt} & The prompt \\
-- \var{prog} & Program name or a function that returns it when called. \\
-- \end{tabularx}
function querylib.make_execfile_fn(prompt, prog)
    local function handle_execwith(frame, str)
        local p, err=getprog(prog)
        if p then
            querylib.last_dir=string.gsub(str, "^(.*/)[^/]-$", "%1")
            exec_in(frame, p.." "..string.shell_safe(str))
        else
            query_fwarn(frame, err)
        end
    end
    
    return querylib.make_frame_fn(prompt, querylib.get_initdir, 
                                  handle_execwith, querylib.file_completor)
end


-- }}}


-- Simple queries for internal actions {{{


function querylib.getws(obj)
    while obj~=nil do
        if obj_is(obj, "WGenWS") then
            return obj
        end
        obj=obj:manager()
    end
end

function querylib.gotoclient_handler(frame, str)
    local cwin=lookup_clientwin(str)
    
    if cwin==nil then
        query_fwarn(frame, string.format("Could not find client window named"
                                         .. ' "%s".', str))
    else
        cwin:goto()
    end
end

function querylib.attachclient_handler(frame, str)
    local cwin=lookup_clientwin(str)
    
    if cwin==nil then
        query_fwarn(frame, string.format("Could not find client window named"
                                         .. ' "%s".', str))
        return
    end
    
    if frame:rootwin_of()~=cwin:rootwin_of() then
        query_fwarn(frame, "Cannot attach: not on same root window.")
        return
    end
    
    frame:attach(cwin, { switchto = true })
end

function querylib.lookup_workspace_classes()
    local classes={}
    
    for k, v in _G do
        local m=getmetatable(v)
        if m and m.__index==WGenWS then
            table.insert(classes, k)
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

function querylib.workspace_handler(frame, name)
    local ws=lookup_workspace(name)
    if ws then
        ws:goto()
        return
    end
    
    local classes=querylib.lookup_workspace_classes()
    
    local function completor(wedln, what)
        local results=querylib.complete_from_list(classes, what)
        wedln:set_completions(results)
    end
    
    local function handler(cls)
        local scr=frame:screen_of()
        if not scr then
            query_fwarn(frame, "Unable to create workspace: no screen.")
            return
        end
        
        if not cls or cls=="" then
            cls=default_ws_type
        end
    
        err=collect_errors(function()
                               ws=scr:attach_new({ 
                                   type=cls, 
                                   name=name, 
                                   switchto=true 
                               })
                           end)
        if not ws then
            query_fwarn(frame, err or "Unknown error")
        end
    end

    local prompt="Workspace type ("..default_ws_type.."):"
    
    query_query(frame, prompt, "", handler, completor)
end

--DOC
-- This query asks for the name of a client window and attaches
-- it to the frame the query was opened in. It uses the completion
-- function \fnref{complete_clientwin}.
querylib.query_gotoclient=querylib.make_frame_fn(
    "Go to window:", nil,
    querylib.gotoclient_handler,
    querylib.make_completor(complete_clientwin)
)

--DOC
-- This query asks for the name of a client window and switches
-- focus to the one entered. It uses the completion function
-- \fnref{complete_clientwin}.
querylib.query_attachclient=querylib.make_frame_fn(
    "Attach window:", nil,
    querylib.attachclient_handler, 
    querylib.make_completor(complete_clientwin)
)

--DOC
-- This query asks for the name of a workspace. If a workspace
-- (an object inheriting \type{WGenWS}) with such a name exists,
-- it will be switched to. Otherwise a new workspace with the
-- entered name will be created. The default class for such a workspace
-- has been \emph{temporarily} hardcoded to \type{WIonWS}. By prefixing
-- the input string with ''classname:'' it is possible to create other 
-- kinds of workspaces.
querylib.query_workspace=querylib.make_frame_fn(
    "Go to or create workspace:", nil, 
    querylib.workspace_handler,
    querylib.make_completor(complete_workspace)
)

--DOC
-- This query asks whether the user wants to have Ioncore exit.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
querylib.query_exit=querylib.make_yesno_fn(
    "Exit Ion (y/n)?", exit_wm
)

--DOC
-- This query asks whether the user wants restart Ioncore.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
querylib.query_restart=querylib.make_yesno_fn(
    "Restart Ion (y/n)?", restart_wm
)

--DOC
-- This function asks for a name new for the frame where the query
-- was created.
querylib.query_renameframe=querylib.make_rename_fn(
    "Frame name:", function(frame) return frame end
)

--DOC
-- This function asks for a name new for the workspace on which the
-- query resides.
querylib.query_renameworkspace=querylib.make_rename_fn(
    "Workspace name:", querylib.getws
)


-- }}}


-- Queries that start external programs {{{


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

-- How many characters of result data to completions do we allow?
querylib.RESULT_DATA_LIMIT=10*1024^2

function querylib.file_completor(wedln, str, wp)
    local function receive_data(str)
        local data=""
        
        while str do
            data=data .. str
            if string.len(data)>querylib.RESULT_DATA_LIMIT then
                error("Too much result data")
            end
            str=coroutine.yield()
        end
        
        
        local results={}
        
        -- ion-completefile will return possible common part of path on
        -- the first line and the entries in that directory on the
        -- following lines.
        for a in string.gfind(data, "([^\n]*)\n") do
            table.insert(results, a)
        end
        
        results.common_part=results[1]
        table.remove(results, 1)
        
        wedln:set_completions(results)
    end
    
    local ic=lookup_script("ion-completefile")
    if ic then
        popen_bgread(ic..(wp or " ")..string.shell_safe(str),
                     coroutine.wrap(receive_data))
    end
end


--DOC
-- Asks for a file to be edited. It uses the script \file{ion-edit} to
-- start a program to edit the file. This script uses \file{run-mailcap}
-- by default, but if you don't have it, you may customise the script.
querylib.query_editfile=querylib.make_execfile_fn(
    "Edit file:", querylib.make_script_lookup_fn("ion-edit")
)

--DOC
-- Asks for a file to be viewed. It uses the script \file{ion-view} to
-- start a program to view the file. This script uses \file{run-mailcap}
-- by default, but if you don't have it, you may customise the script.
querylib.query_runfile=querylib.make_execfile_fn(
    "View file:", querylib.make_script_lookup_fn("ion-view")
)


function querylib.exec_completor(wedln, str)
    querylib.file_completor(wedln, str, " -wp ")
end

function querylib.exec_handler(frame, cmd)
    if string.sub(cmd, 1, 1)==":" then
        local ix=get_script_warn(frame, "ion-runinxterm")
        if not ix then return end
        cmd=ix.." "..string.sub(cmd, 2)
    end
    exec_in(frame, cmd)
end

--DOC
-- This function asks for a command to execute with \file{/bin/sh}.
-- If the command is prefixed with a colon (':'), the command will
-- be run in an XTerm (or other terminal emulator) using the script
-- \file{ion-runinxterm}.
querylib.query_exec=querylib.make_frame_fn(
    "Run:", nil, querylib.exec_handler, querylib.exec_completor
)


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

querylib.do_query_ssh=querylib.make_execwith_fn(
    "SSH to:", nil, 
    querylib.make_script_lookup_fn("ion-ssh"),
    querylib.make_completor(querylib.complete_ssh)
)

--DOC
-- This query asks for a host to connect to with SSH. It starts
-- up ssh in a terminal using \file{ion-ssh}. Hosts to tab-complete
-- are read from \file{\~{}/.ssh/known\_hosts}.
function querylib.query_ssh(mplex)
    querylib.get_known_hosts(mplex)
    return querylib.do_query_ssh(mplex)
end


-- Use weak references to cache found manuals.
querylib.mancache={}
setmetatable(querylib.mancache, {__mode="v"})

function querylib.man_completor(wedln, str)
    local function set_completions(manuals)
        local results=querylib.complete_from_list(manuals, str)
        wedln:set_completions(results)
    end

    if querylib.mancache.manuals then
        set_completions(querylib.mancache.manuals)
        return
    end    
    
    local function receive_data(str)
        local data = "\n"
        
        while str do
            data=data .. str
            if string.len(data)>querylib.RESULT_DATA_LIMIT then
                error("Too much result data")
            end
            str=coroutine.yield()
        end
        
        local manuals={}
        
        for a in string.gfind(data, "[^\n]*/([^/.\n]+)%.%d[^\n]*\n") do
            table.insert(manuals, a)
        end

        querylib.mancache.manuals=manuals
        
        set_completions(manuals)
    end
    
    local dirs=table.concat(query_man_path, " ")
    if dirs==nil then
        return
    end
    
    popen_bgread("find "..dirs.." -type f -o -type l", 
                 coroutine.wrap(receive_data))
end


--DOC
-- This query asks for a manual page to display. It uses the command
-- \file{ion-man} to run \file{man} in a terminal emulator. By customizing
-- this script it is possible use some other man page viewer. To enable
-- tab-completion you must list paths with manuals in the table
-- \var{query_man_path}. For example,
--\begin{verbatim}
--query_man_path = {
--    "/usr/local/man","/usr/man",
--    "/usr/share/man", "/usr/X11R6/man",
--}
--\end{verbatim}
querylib.query_man=querylib.make_execwith_fn(
    "Manual page (ion):", "ion",
    querylib.make_script_lookup_fn("ion-man"),
    querylib.man_completor
)


-- }}}


-- Lua code execution {{{


function querylib.create_run_env(mplex)
    local origenv=getfenv()
    local meta={__index=origenv, __newindex=origenv}
    local env={_=mplex, arg={mplex, mplex:current()}}
    setmetatable(env, meta)
    return env
end
    
function querylib.do_handle_lua(mplex, env, code)
    local f, err=loadstring(code)
    if not f then
        query_fwarn(mplex, err)
        return
    end
    setfenv(f, env)
    err=collect_errors(f)
    if err then
        query_fwarn(mplex, err)
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

    local function handle(code)
        return querylib.do_handle_lua(mplex, env, code)
    end
    
    query_query(mplex, "Lua code to run: ", nil, handle, complete)
end


-- }}}


-- Miscellaneous {{{


--DOC 
-- Display an "About Ion" message in \var{mplex}.
function querylib.show_aboutmsg(mplex)
    query_message(mplex, ioncore_aboutmsg())
end


-- }}


-- Mark ourselves loaded.
_LOADED["querylib"]=true
