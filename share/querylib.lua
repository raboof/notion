--
-- ion/share/querylib.lua -- Some common queries for Ion
-- 
-- Copyright (c) Tuomo Valkonen 2003.
-- 
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

if QueryLib~=nil then
    return
end

QueryLib={}


-- Functions to generate functions {{{

function QueryLib.make_completor(completefn)
    local function completor(wedln, str)
        wedln_set_completions(wedln, completefn(str))
    end
    return completor
end

    
function QueryLib.make_simple_fn(prompt, initfn, handler, completor)
    local function query_it(frame)
        local initvalue;
        if initfn then
            initvalue=initfn()
        end
        query_query(frame, prompt, initvalue or "", handler, completor)
    end
    return query_it
end

function QueryLib.make_frame_fn(prompt, initfn, handler, completor)
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

function QueryLib.make_rename_fn(prompt, getobj)
    local function query_it(frame)
        local obj=getobj(frame)
        local function handle_it(str)
            region_set_name(obj, str)
        end
        query_query(frame, prompt, region_name(obj) or "", handle_it, nil)
    end
    return query_it
end

function QueryLib.make_yesno_handler(fn)
    local function handle_yesno(...)
        local n=table.getn(arg)
        if n==0 then return end
        if arg[n]=="y" or arg[n]=="Y" or arg[n]=="yes" then
            table.remove(arg, n)
            fn(unpack(arg))
        end
    end
    return handle_yesno
end

function QueryLib.make_yesno_fn(prompt, handler)
    return QueryLib.make_frame_fn(prompt, nil,
                                  QueryLib.make_yesno_handler(handler),
                                  nil)
end

function QueryLib.make_script_lookup_fn(script)
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
    local s, err=QueryLib.make_script_lookup_fn(script)()
    if not s then
        query_fwarn(frame, s)
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

function QueryLib.make_execwith_fn(prompt, init, prog, completor)
    local function handle_execwith(frame, str)
        local p, err=getprog(prog)
        if p then
            exec_in_frame(frame, p.." "..str)
        else
            query_fwarn(frame, err)
        end
    end
    return QueryLib.make_frame_fn(prompt, init, handle_execwith, completor)
end

function QueryLib.make_execfile_fn(prompt, init, prog, completor)
    local function handle_execwith(frame, str)
        local p, err=getprog(prog)
        if p then
            QueryLib.last_dir=string.gsub(str, "^(.*/)[^/]-$", "%1")
            exec_in_frame(frame, getprog(prog) .. " " .. str)
        else
            query_fwarn(frame, err)
        end
    end
    return QueryLib.make_frame_fn(prompt, init, handle_execwith, completor)
end

-- }}}


-- Simple handlers and completors {{{

function QueryLib.exec_handler(frame, cmd)
    if string.sub(cmd, 1, 1)==":" then
        local ix=get_script_warn(frame, "ion-runinxterm")
        if not ix then return end
        cmd=ix.." "..string.sub(cmd, 2)
    end
    exec_in_frame(frame, cmd)
end

function QueryLib.getws(obj)
    while obj~=nil do
        if obj_is(obj, "WGenWS") then
            return obj
        end
        obj=region_manager(obj)
    end
end

function QueryLib.complete_ssh(str)
    if string.len(str)==0 then
        return query_ssh_hosts
    end
    
    local res={}
    for _, v in ipairs(query_ssh_hosts) do
    	local s, e=string.find(v, str, 1, true)
	if s==1 and e>=1 then
            table.insert(res, v)
        end
    end
    return res
end

function QueryLib.gotoclient_handler(frame, str)
    local cwin=lookup_clientwin(str)
    
    if cwin==nil then
        query_fwarn(frame, string.format("Could not find client window named"
                                         .. ' "%s".', str))
    else
        region_goto(cwin)
    end
end

function QueryLib.attachclient_handler(frame, str)
    local cwin=lookup_clientwin(str)
    
    if cwin==nil then
        query_fwarn(frame, string.format("Could not find client window named"
                                         .. ' "%s".', str))
        return
    end
    
    if region_rootwin_of(frame)~=region_rootwin_of(cwin) then
        query_fwarn(frame, "Cannot attach: not on same root window.")
        return
    end
    
    region_manage(frame, cwin, { selected = true })
end

function QueryLib.workspace_handler(frame, name)
    name=string.gsub(name, "^%s*(.-)%s*$", "%1")
    local ws=lookup_workspace(name)
    if ws then
        region_goto(ws)
        return
    end
    
    local scr=region_screen_of(frame)
    if not scr then
        query_fwarn(frame, "Unable to create workspace: no screen.")
        return
    end
    
    local _, _, cls, nam=string.find(name, "^(.-):%s*(.-)%s*$")
    if not cls then
        -- TODO: This hardcoding should be removed when the C->Lua interfac
        -- is more object-oriented and we can scan the available classes. 
        -- I can't be bothered to create such a function at the moment.
        cls=default_ws_type
    else
        name=nam
    end
    
    ws=region_manage_new(scr, { type=cls, name=name, selected=true })
    if not ws then
        query_fwarn(frame, "Failed to create workspace")
    end
    
end

-- }}}


-- Lua code execution and completion {{{

function QueryLib.handler_lua(frame, code)
    local f, err=loadstring(code)
    if not f then
        query_fwarn(frame, err)
        return
    end
    -- Create a new environment with parameters
    local origenv=getfenv(f)
    local meta={__index=origenv, __newindex=origenv}
    local env={_=frame, arg={frame, genframe_current(frame)}}
    setmetatable(env, meta)
    setfenv(f, env)
    err=collect_errors(f)
    if err then
        query_fwarn(frame, err)
    end
end

function QueryLib.complete_lua(str)
    local comptab=_G;
    
    -- Get the variable to complete, including containing tables.
    -- This will also match string concatenations and such because
    -- Lua's regexps don't support optional subexpressions, but we
    -- handle them in the next step.
    local _, _, tocomp=string.find(str, "([%w_.:]*)$")
    
    -- Descend into tables
    if tocomp and string.len(tocomp)>=1 then
        for t in string.gfind(tocomp, "([^.:]*)[.:]") do
            if string.len(t)==0 then
                comptab=_G;
            elseif comptab then
                if type(comptab[t])=="table" then
                    comptab=comptab[t]
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

    -- TODO: metatable scanning
    
    for k in comptab do
        if type(k)=="string" then
            if string.sub(k, 1, l)==tocomp then
                table.insert(compl, k)
            end
        end
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

-- }}}


-- More complex completors that start external programs {{{

function QueryLib.get_initdir()
    if QueryLib.last_dir then
        return QueryLib.last_dir
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
QueryLib.RESULT_DATA_LIMIT=10*1024^2

-- Use weak references to cache found manuals.
QueryLib.mancache={}
setmetatable(QueryLib.mancache, {__mode="v"})

function QueryLib.man_completor(wedln, str)
    local function set_completions(manuals)
        local results={}
        local len=string.len(str)
        if len==0 then
            results=manuals
        else
            for _, m in manuals do
                if string.sub(m, 1, len)==str then
                    table.insert(results, m)
                end
            end
        end
        wedln_set_completions(wedln, results)
    end

    if QueryLib.mancache.manuals then
        set_completions(QueryLib.mancache.manuals)
        return
    end    
    
    local function receive_data(str)
        local data = "\n"
        
        while str do
            data=data .. str
            if string.len(data)>QueryLib.RESULT_DATA_LIMIT then
                error("Too much result data")
            end
            str=coroutine.yield()
        end
        
        local manuals={}
        
        for a in string.gfind(data, "[^\n]*/([^/.\n]+)%.%d[^\n]*\n") do
            table.insert(manuals, a)
        end

        QueryLib.mancache.manuals=manuals
        
        set_completions(manuals)
    end
    
    local dirs=table.concat(query_man_path, " ")
    if dirs==nil then
        return
    end
    
    popen_bgread("find "..dirs.." -type f", coroutine.wrap(receive_data))
end


function QueryLib.file_completor(wedln, str, wp)
    local function receive_data(str)
        local data=""
        
        while str do
            data=data .. str
            if string.len(data)>QueryLib.RESULT_DATA_LIMIT then
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
        
        wedln_set_completions(wedln, results)
    end
    
    str=string.gsub(str, "'", "'\\''")
    local ic=lookup_script("ion-completefile")
    if ic then
        popen_bgread(ic.." '"..str.."' "..(wp or ""), 
                     coroutine.wrap(receive_data))
    end
end


function QueryLib.exec_completor(wedln, str)
    QueryLib.file_completor(wedln, str, "-wp")
end

-- }}}


-- The queries {{{

-- Internal operations

--DOC
-- This query asks for the name of a client window and attaches
-- it to the frame the query was opened in. It uses the completion
-- function \fnref{complete_clientwin}.
QueryLib.query_gotoclient=QueryLib.make_frame_fn(
    "Go to window:", nil,
    QueryLib.gotoclient_handler,
    QueryLib.make_completor(complete_clientwin)
)

--DOC
-- This query asks for the name of a client window and switches
-- focus to the one entered. It uses the completion function
-- \fnref{complete_clientwin}.
QueryLib.query_attachclient=QueryLib.make_frame_fn(
    "Attach window:", nil,
    QueryLib.attachclient_handler, 
    QueryLib.make_completor(complete_clientwin)
)

--DOC
-- This query asks for the name of a workspace. If a workspace
-- (an object inheriting \type{WGenWS}) with such a name exists,
-- it will be switched to. Otherwise a new workspace with the
-- entered name will be created. The default class for such a workspace
-- has been \emph{temporarily} hardcoded to \type{WIonWS}. By prefixing
-- the input string with ''classname:'' it is possible to create other 
-- kinds of workspaces.
QueryLib.query_workspace=QueryLib.make_frame_fn(
    "Go to or create workspace:", nil, 
    QueryLib.workspace_handler,
    QueryLib.make_completor(complete_workspace)
)

--DOC
-- This query asks whether the user wants to have Ioncore exit.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
QueryLib.query_exit=QueryLib.make_yesno_fn(
    "Exit Ion (y/n)?", exit_wm
)

--DOC
-- This query asks whether the user wants restart Ioncore.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
QueryLib.query_restart=QueryLib.make_yesno_fn(
    "Restart Ion (y/n)?", restart_wm
)

--DOC
-- This function asks for a name new for the frame where the query
-- was created.
QueryLib.query_renameframe=QueryLib.make_rename_fn(
    "Frame name:", function(frame) return frame end
)

--DOC
-- This function asks for a name new for the workspace on which the
-- query resides.
QueryLib.query_renameworkspace=QueryLib.make_rename_fn(
    "Workspace name:", QueryLib.getws
)

-- Queries for starting external programs

--DOC
-- This function asks for a command to execute with \file{/bin/sh}.
-- If the command is prefixed with a colon (':'), the command will
-- be run in an XTerm (or other terminal emulator) using the script
-- \file{ion-runinxterm}.
QueryLib.query_exec=QueryLib.make_frame_fn(
    "Run:", nil, QueryLib.exec_handler, QueryLib.exec_completor
)

--DOC
-- This query asks for a host to connect to with SSH. It starts
-- up ssh in a terminal using \file{ion-ssh}. To enable tab completion,
-- put the names of often-used hosts in the table \var{query_ssh_hosts}.
QueryLib.query_ssh=QueryLib.make_execwith_fn(
    "SSH to:", nil, 
    QueryLib.make_script_lookup_fn("ion-ssh"),
    QueryLib.make_completor(QueryLib.complete_ssh)
)

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
QueryLib.query_man=QueryLib.make_execwith_fn(
    "Manual page (ion):", nil,
    QueryLib.make_script_lookup_fn("ion-man"),
    QueryLib.man_completor
)

--DOC
-- Asks for a file to be edited. It uses the script \file{ion-edit} to
-- start a program to edit the file. This script uses \file{run-mailcap}
-- by default, but if you don't have it, you may customise the script.
QueryLib.query_editfile=QueryLib.make_execfile_fn(
    "Edit file:", QueryLib.get_initdir, 
    QueryLib.make_script_lookup_fn("ion-edit"),
    QueryLib.file_completor
)

--DOC
-- Asks for a file to be viewed. It uses the script \file{ion-view} to
-- start a program to view the file. This script uses \file{run-mailcap}
-- by default, but if you don't have it, you may customise the script.
QueryLib.query_runfile=QueryLib.make_execfile_fn(
    "View file:", QueryLib.get_initdir, 
    QueryLib.make_script_lookup_fn("ion-view"),
    QueryLib.file_completor
)

-- Lua code execution

--DOC
-- This query asks for Lua code to execute. It sets the variable '\var{\_}'
-- in the local environment of the string to point to the frame where the
-- query was created. It also sets the table \var{arg} in the local
-- environment to \code{\{_, genframe_current(_)\}}.
QueryLib.query_lua=QueryLib.make_frame_fn(
    "Lua code to run:", nil,
    QueryLib.handler_lua,
    QueryLib.make_completor(QueryLib.complete_lua)
)

-- }}}
