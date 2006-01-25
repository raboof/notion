--
-- ion/query/mod_query.lua -- Some common queries for Ion
-- 
-- Copyright (c) Tuomo Valkonen 2004-2006.
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


local DIE_TIMEOUT_ERRORCODE=10 -- 10 seconds
local DIE_TIMEOUT_NO_ERRORCODE=2 -- 2 seconds


-- Generic helper functions {{{


function mod_query.make_completor(completefn)
    local function completor(cp, str, point)
        cp:set_completions(completefn(str, point))
    end
    return completor
end


--DOC
-- Low-level query routine. \var{mplex} is the \type{WMPlex} to display
-- the query in, \var{prompt} the prompt string, and \var{initvalue}
-- the initial contents of the query box. \var{handler} is a function
-- that receives (\var{mplex}, result string) as parameter when the
-- query has been succesfully completed, \var{completor} the completor
-- routine which receives a (\var{cp}, \var{str}, \var{point}) as parameters.
-- The parameter \var{str} is the string to be completed and \var{point}
-- cursor's location within it. Completions should be eventually,
-- possibly asynchronously, set with \fnref{WComplProxy.set_completions} 
-- on \var{cp}.
function mod_query.query(mplex, prompt, initvalue, handler, completor,
                         context)
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
    local wedln=mod_query.do_query(mplex, prompt, initvalue, 
                                   handle_it, completor)
    if context then
        wedln:set_context(context)
    end
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
    return mod_query.query(mplex, prompt, nil, handler_yesno, nil,
                           "yesno")
end


local errdata={}

local function maybe_finish(pid)
    local t=errdata[pid]

    if t and t.closed and t.dietime then
        errdata[pid]=nil
        local tmd=os.difftime(t.dietime, t.starttime)
        --if tmd<DIE_TIMEOUT_ERRORCODE and t.signaled then
        --    local msg=TR("Program received signal ")..t.termsig.."\n"
        --    mod_query.warn(t.mplex, msg..(t.errs or ""))
        --else
        if ((tmd<DIE_TIMEOUT_ERRORCODE and (t.hadcode or t.signaled)) or
                (tmd<DIE_TIMEOUT_NO_ERRORCODE)) and t.errs then
            mod_query.warn(t.mplex, t.errs)
        end
    end
end


local badsig_={4, 5, 6, 7, 8, 11}
local badsig={}
for _, v in badsig_ do 
    badsig[v]=true 
end

local function chld_handler(p)
    local t=errdata[p.pid]
    if t then
        t.dietime=os.time()
        t.signaled=(p.signaled and badsig[p.termsig])
        t.termsig=p.termsig
        t.hadcode=(p.exited and p.exitstatus~=0)
        maybe_finish(pid)
    end
end

ioncore.get_hook("ioncore_sigchld_hook"):add(chld_handler)

function mod_query.exec_on_merr(mplex, cmd)
    local pid
    
    local function monitor(str)
        if pid then
            local t=errdata[pid]
            if t then
                if str then
                    t.errs=(t.errs or "")..str
                else
                    t.closed=true
                    maybe_finish(pid)
                end
            end
        end
    end
    
    local function timeout()
        errdata[pid]=nil
    end
    
    pid=ioncore.exec_on(mplex, cmd, monitor)
    
    if pid<=0 then
        return
    end

    local tmr=ioncore.create_timer();
    local tmd=math.max(DIE_TIMEOUT_NO_ERRORCODE, DIE_TIMEOUT_ERRORCODE)
    local now=os.time()  
    tmr:set(tmd*1000, timeout)
    
    errdata[pid]={tmr=tmr, mplex=mplex, starttime=now}
end


function mod_query.file_completor(wedln, str)
    local ic=ioncore.lookup_script("ion-completefile")
    if ic then
        mod_query.popen_completions(wedln,
                                   ic.." "..string.shell_safe(str),
                                   "")
    end
end


function mod_query.query_execfile(mplex, prompt, prog)
    assert(prog~=nil)
    local function handle_execwith(mplex, str)
        mod_query.exec_on_merr(mplex, prog.." "..string.shell_safe(str))
    end
    return mod_query.query(mplex, prompt, mod_query.get_initdir(mplex),
                           handle_execwith, mod_query.file_completor,
                           "filename")
end


function mod_query.query_execwith(mplex, prompt, dflt, prog, completor,
                                  context, noquote)
    local function handle_execwith(frame, str)
        if not str or str=="" then
            str=dflt
        end
        local args=(noquote and str or string.shell_safe(str))
        mod_query.exec_on_merr(mplex, prog.." "..args)
    end
    return mod_query.query(mplex, prompt, nil, handle_execwith, completor,
                           context)
end


function mod_query.get_initdir(mplex)
    --if mod_query.last_dir then
    --    return mod_query.last_dir
    --end
    local wd=(ioncore.get_dir_for(mplex) or os.getenv("PWD"))
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
-- the command should on the first line write the \var{common_beg} 
-- parameter of \fnref{WComplProxy.set_completions} and a single actual 
-- completion on each of the successive lines.
function mod_query.popen_completions(cp, cmd, beg, fn)
    
    local pst={cp=cp, maybe_stalled=0}
    
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
                error(TR("Too much result data"))
            end

            data=string.gsub(data..str, "([^\n]*)\n",
                             function(a)
                                 -- ion-completefile will return possible 
                                 -- common part of path on  the first line 
                                 -- and the entries in that directory on the
                                 -- following lines.
                                 if not results.common_beg then
                                     results.common_beg=(beg or "")..a
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
        
        if not results.common_beg then
            results.common_beg=beg
        end
        
        (fn or WComplProxy.set_completions)(cp, results)
        
        pipes[rcv]=nil
        results={}
        
        collectgarbage()
    end
    
    local found_clean=false
    
    for k, v in pipes do
        if v.cp==cp then
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


-- }}}


-- Simple queries for internal actions {{{


function mod_query.call_warn(mplex, fn)
    local err = collect_errors(fn)
    if err then
        mod_query.warn(mplex, err)
    end
    return err
end


function mod_query.complete_name(str, list)
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
    return mod_query.complete_name(str, ioncore.clientwin_list())
end

function mod_query.complete_workspace(str)
    return mod_query.complete_name(str, ioncore.region_list("WGenWS"))
end

function mod_query.complete_region(str)
    return mod_query.complete_name(str, ioncore.region_list())
end


function mod_query.gotoclient_handler(frame, str)
    local cwin=ioncore.lookup_clientwin(str)
    
    if cwin==nil then
        mod_query.warn(frame, TR("Could not find client window %s.", str))
    else
        cwin:goto()
    end
end

function mod_query.attachclient_handler(frame, str)
    local cwin=ioncore.lookup_clientwin(str)
    
    if not cwin then
        mod_query.warn(frame, TR("Could not find client window %s.", str))
    elseif frame:rootwin_of()~=cwin:rootwin_of() then
        mod_query.warn(frame, TR("Cannot attach: different root windows."))
    elseif cwin:manager()==frame then
        cwin:goto()
    else
        mod_query.call_warn(frame, 
                            function() 
                                frame:attach(cwin, { switchto = true }) 
                            end)
    end
end


function mod_query.workspace_handler(mplex, name)
    local ws=ioncore.lookup_region(name, "WGenWS")
    if ws then
        ws:goto()
        return
    end
    
    local classes=mod_query.lookup_workspace_classes()
    
    local function completor(cp, what)
        local results=mod_query.complete_from_list(classes, what)
        cp:set_completions(results)
    end
    
    local function handler(mplex, cls)
        local scr=mplex:screen_of()
        if not scr then
            mod_query.warn(mplex, TR("Unable to create workspace: no screen."))
            return
        end
        
        if not cls or cls=="" then
            cls=ioncore.get().default_ws_type
        end

        mod_query.call_warn(mplex, 
                            function()
                                ws=scr:attach_new({ 
                                    type=cls, 
                                    name=name, 
                                    switchto=true 
                                 })
                                 if not ws then
                                     error(TR("Unknown error"))
                                 end
                            end)
    end
    
    local defcls=ioncore.get().default_ws_type
    local prompt=TR("Workspace type (%s):", defcls or TR("none"))
    
    mod_query.query(mplex, prompt, "", handler, completor,
                    "workspacename")
end


--DOC
-- This query asks for the name of a client window and attaches
-- it to the frame the query was opened in. It uses the completion
-- function \fnref{ioncore.complete_clientwin}.
function mod_query.query_gotoclient(mplex)
    mod_query.query(mplex, TR("Go to window:"), nil,
                    mod_query.gotoclient_handler,
                    mod_query.make_completor(mod_query.complete_clientwin),
                    "windowname")
end

--DOC
-- This query asks for the name of a client window and switches
-- focus to the one entered. It uses the completion function
-- \fnref{ioncore.complete_clientwin}.
function mod_query.query_attachclient(mplex)
    mod_query.query(mplex, TR("Attach window:"), nil,
                    mod_query.attachclient_handler, 
                    mod_query.make_completor(mod_query.complete_clientwin),
                    "windowname")
end


--DOC
-- This query asks for the name of a workspace. If a workspace
-- (an object inheriting \type{WGenWS}) with such a name exists,
-- it will be switched to. Otherwise a new workspace with the
-- entered name will be created and the user will be queried for
-- the type of the workspace.
function mod_query.query_workspace(mplex)
    mod_query.query(mplex, TR("Go to or create workspace:"), nil, 
                    mod_query.workspace_handler,
                    mod_query.make_completor(mod_query.complete_workspace),
                    "workspacename")
end


--DOC
-- This query asks whether the user wants to exit Ion (no session manager)
-- or close the session (running under a session manager that supports such
-- requests). If the answer is 'y', 'Y' or 'yes', so will happen.
function mod_query.query_shutdown(mplex)
    mod_query.query_yesno(mplex, TR("Exit Ion/Shutdown session (y/n)?"),
                         ioncore.shutdown)
end


--DOC
-- This query asks whether the user wants restart Ioncore.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
function mod_query.query_restart(mplex)
    mod_query.query_yesno(mplex, TR("Restart Ion (y/n)?"), ioncore.restart)
end


--DOC
-- This function asks for a name new for the frame where the query
-- was created.
function mod_query.query_renameframe(frame)
    mod_query.query(frame, TR("Frame name:"), frame:name(),
                    function(frame, str) frame:set_name(str) end,
                    nil, "framename")
end


--DOC
-- This function asks for a name new for the workspace on which the
-- query resides.
function mod_query.query_renameworkspace(mplex)
    local ws=ioncore.find_manager(mplex, "WGenWS")
    mod_query.query(mplex, TR("Workspace name:"), ws:name(),
                    function(mplex, str) ws:set_name(str) end,
                    nil, "framename")
end


-- }}}


-- Run/view/edit {{{


--DOC
-- Asks for a file to be edited. This script uses 
-- \command{run-mailcap --mode=edit} by default, but you may provide an
-- alternative script to use. The default prompt is "Edit file:" (translated).
function mod_query.query_editfile(mplex, script, prompt)
    mod_query.query_execfile(mplex, 
                             prompt or TR("Edit file:"), 
                             script or "run-mailcap --action=edit")
end


--DOC
-- Asks for a file to be viewed. This script uses 
-- \command{run-mailcap --action=view} by default, but you may provide an
-- alternative script to use. The default prompt is "View file:" (translated).
function mod_query.query_runfile(mplex, script, prompt)
    mod_query.query_execfile(mplex, 
                             prompt or TR("View file:"), 
                             script or "run-mailcap --action=view")

end


local function isspace(s)
    return string.find(s, "^%s*$")~=nil
end


local function break_cmdline(str, no_ws)
    local st, en, beg, rest, ch, rem
    local res={""}

    local function ins(str)
        local n=table.getn(res)
        if string.find(res[n], "^%s+$") then
            table.insert(res, str)
        else
            res[n]=res[n]..str
        end
    end

    local function ins_space(str)
        local n=table.getn(res)
        if no_ws then
            if res[n]~="" then
                table.insert(res, "")
            end
        else
            if isspace(res[n]) then
                res[n]=res[n]..str
            else
                table.insert(res, str)
            end
        end
    end

    -- Handle terminal startup syntax
    st, en, beg, ch, rest=string.find(str, "^(%s*)(:+)(.*)")
    if beg then
        if string.len(beg)>0 then
            ins_space(beg)
        end
        ins(ch)
        ins_space("")
        str=rest
    end

    while str~="" do
        st, en, beg, rest, ch=string.find(str, "^(.-)(([%s'\"\\|])(.*))")
        if not beg then
            ins(str)
            break
        end

        ins(beg)
        str=rest
        
        local sp=false
        
        if ch=="\\" then
            st, en, beg, rest=string.find(str, "^(\\.)(.*)")
        elseif ch=='"' then
            st, en, beg, rest=string.find(str, "^(\".-[^\\]\")(.*)")
            
            if not beg then
                st, en, beg, rest=string.find(str, "^(\"\")(.*)")
            end
        elseif ch=="'" then
            st, en, beg, rest=string.find(str, "^('.-')(.*)")
        else
            if ch=='|' then
                ins_space('')
                ins(ch)
            else -- ch==' '
                ins_space(ch)
            end
            st, en, beg, rest=string.find(str, "^.(%s*)(.*)")
            assert(beg and rest)
            ins_space(beg)
            sp=true
            str=rest
        end
        
        if not sp then
            if not beg then
                beg=str
                rest=""
            end
            ins(beg)
            str=rest
        end
    end
    
    return res
end


local function unquote(str)
    str=string.gsub(str, "^['\"]", "")
    str=string.gsub(str, "([^\\])['\"]", "%1")
    str=string.gsub(str, "\\(.)", "%1")
    return str
end


local function quote(str)
    return string.gsub(str, "([%(%)\"'\\%*%?%[%]%| ])", "\\%1")
end


local function find_point(strs, point)
    for i, s in ipairs(strs) do
        point=point-string.len(s)
        if point<=1 then
            return i
        end
    end
    return table.getn(strs)
end


function mod_query.exec_completor(wedln, str, point)
    local parts=break_cmdline(str)
    local complidx=find_point(parts, point+1)
    
    local s_compl, s_beg, s_end="", "", ""
    
    if complidx==1 and string.find(parts[1], "^:+$") then
        complidx=complidx+1
    end
    
    if string.find(parts[complidx], "[^%s]") then
        s_compl=unquote(parts[complidx])
    end
    
    for i=1, complidx-1 do
        s_beg=s_beg..parts[i]
    end
    
    for i=complidx+1, table.getn(parts) do
        s_end=s_end..parts[i]
    end
    
    local wp=" "
    if complidx==1 or (complidx==2 and isspace(parts[1])) then
        wp=" -wp "
    elseif string.find(parts[1], "^:+$") then
        if complidx==2 then
            wp=" -wp "
        elseif string.find(parts[2], "^%s*$") then
            if complidx==3 then
                wp=" -wp "
            end
        end
    end

    local function set_fn(cp, res)
        res=table.map(quote, res)
        res.common_beg=s_beg..(res.common_beg or "")
        res.common_end=(res.common_end or "")..s_end
        cp:set_completions(res)
    end

    local ic=ioncore.lookup_script("ion-completefile")
    if ic then
        mod_query.popen_completions(wedln,
                                   ic..wp..string.shell_safe(s_compl),
                                   "", set_fn)
    end
end


local cmd_overrides={}


--DOC
-- Define a command override for the \fnrefx{mod_query}{query_exec} query.
function mod_query.defcmd(cmd, fn)
    cmd_overrides[cmd]=fn
end


function mod_query.exec_handler(mplex, cmdline)
    local parts=break_cmdline(cmdline, true)
    local cmd=table.remove(parts, 1)
    
    if cmd_overrides[cmd] then
        cmd_overrides[cmd](mplex, table.map(unquote, parts))
    elseif cmd~="" then
        mod_query.exec_on_merr(mplex, cmdline)
    end
end


--DOC
-- This function asks for a command to execute with \file{/bin/sh}.
-- If the command is prefixed with a colon (':'), the command will
-- be run in an XTerm (or other terminal emulator) using the script
-- \file{ion-runinxterm}. Two colons ('::') will ask you to press 
-- enter after the command has finished.
function mod_query.query_exec(mplex)
    mod_query.query(mplex, TR("Run:"), nil, mod_query.exec_handler, 
                    mod_query.exec_completor,
                    "run")
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
        warn(TR("Failed to open ~/.ssh/known_hosts"))
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


mod_query.hostnicks={}

function mod_query.get_hostnicks(mplex)
    mod_query.hostnicks={}
    local f
    local substr, pat, patterns
    local h=os.getenv("HOME")

    if h then
        f=io.open(h.."/.ssh/config")
    end
    if not f then 
        warn(TR("Failed to open ~/.ssh/config"))
        return
    end

    for l in f:lines() do
        _, _, substr=string.find(l, "^%s*[hH][oO][sS][tT](.*)")
        if substr then
            _, _, pat=string.find(substr, "^%s*[=%s]%s*(%S.*)")
            if pat then
                patterns=pat
            elseif string.find(substr, "^[nN][aA][mM][eE]")
                and patterns then
                for s in string.gfind(patterns, "%S+") do
                    if not string.find(s, "[*?]") then
                        table.insert(mod_query.hostnicks, s)
                    end
                end
            end
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
            return mod_query.ssh_completions
        end
        
        for _, v in ipairs(mod_query.ssh_completions) do
            table.insert(res, user .. v)
        end
        return res
    end
    
    for _, v in ipairs(mod_query.ssh_completions) do
        local s, e=string.find(v, host, 1, true)
        if s==1 and e>=1 then
            table.insert(res, user .. v)
        end
    end
    
    return res
end

mod_query.ssh_completions={}

--DOC
-- This query asks for a host to connect to with SSH. 
-- Hosts to tab-complete are read from \file{\~{}/.ssh/known\_hosts}.
function mod_query.query_ssh(mplex, ssh)
    mod_query.get_known_hosts(mplex)
    mod_query.get_hostnicks(mplex)

    for _, v in ipairs(mod_query.known_hosts) do
        table.insert(mod_query.ssh_completions, v)
    end
    for _, v in ipairs(mod_query.hostnicks) do
        table.insert(mod_query.ssh_completions, v)
    end

    ssh=(ssh or ":ssh")

    local function handle_exec(mplex, str)
        if not (str and string.find(str, "[^%s]")) then
            return
        end
        
        mod_query.exec_on_merr(mplex, ssh.." "..string.shell_safe(str))
    end
    
    return mod_query.query(mplex, TR("SSH to:"), nil, handle_exec,
                           mod_query.make_completor(mod_query.complete_ssh),
                           "ssh")
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
-- This query asks for a manual page to display. By default it runs the
-- \command{man} command in an \command{xterm} using \command{ion-runinxterm},
-- but it is possible to pass another program as the \var{prog} argument.
function mod_query.query_man(mplex, prog)
    local dflt=ioncore.progname()
    mod_query.query_execwith(mplex, TR("Manual page (%s):", dflt), 
                             dflt, prog or ":man", 
                             mod_query.man_completor, "man",
                             true --[[ no quoting ]])
end


-- }}}


-- Lua code execution {{{


function mod_query.create_run_env(mplex)
    local origenv=getfenv()
    local meta={__index=origenv, __newindex=origenv}
    local env={
        _=mplex, 
        _sub=mplex:current(),
        print=my_print
    }
    setmetatable(env, meta)
    return env
end

function mod_query.do_handle_lua(mplex, env, code)
    local print_res
    local function collect_print(...)
        local tmp=""
        local l=table.getn(arg)
        for i=1,l do
            tmp=tmp..tostring(arg[i])..(i==l and "\n" or "\t")
        end
        print_res=(print_res and print_res..tmp or tmp)
    end

    local f, err=loadstring(code)
    if not f then
        mod_query.warn(mplex, err)
        return
    end
    
    env.print=collect_print
    setfenv(f, env)
    
    err=collect_errors(f)
    if err then
        mod_query.warn(mplex, err)
    elseif print_res then
        mod_query.message(mplex, print_res)
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
    _, _, compl.common_beg, tocomp=string.find(str, "(.-)([%w_]*)$")
    
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
    
    local function complete(cp, code)
        cp:set_completions(mod_query.do_complete_lua(env, code))
    end
    
    local function handler(mplex, code)
        return mod_query.do_handle_lua(mplex, env, code)
    end
    
    mod_query.query(mplex, TR("Lua code: "), nil, handler, complete, "lua")
end

-- }}}


-- Menu query {{{

--DOC
-- This query can be used to create a query of a defined menu.
function mod_query.query_menu(mplex, themenu, prompt)
    local _sub=mplex:current()
    local menu=ioncore.evalmenu(themenu, {mplex, _sub})

    if not menu then
        mod_query.warn(mplex, TR("Unknown menu %s.", tostring(themenu)))
        return
    end
    
    if not prompt then
        prompt=menuname..":"
    end

    local function xform_name(n)
        return string.lower(string.gsub(n, "[^%w%.%-_<>]+", "-"))
    end

    local function xform_menu(t, m, p)
        for _, v in ipairs(m) do
            if v.name then
                local n=p..xform_name(v.name)
                while t[n] do
                    n=n.."'"
                end
                t[n]=v
                if v.submenu_fn and not v.noautoexpand then
                    local sm=v.submenu_fn()
                    if sm then
                        xform_menu(t, sm, n.."/")
                    else
                        ioncore.warn(TR("Missing submenu ")..(v.name or ""))
                    end
                end
            end
        end
        return t
    end
    
    local ntab=xform_menu({}, menu, "")
    
    local function complete(str)
        local results={}
        for s, e in ntab do
            if string.find(s, str, 1, true) then
                table.insert(results, s)
            end
        end
        return results
    end
    
    local function handle(mplex, str)
        local e=ntab[str]
        if e then
            if e.func then
                local err=collect_errors(function() 
                                             e.func(mplex, _sub) 
                                         end)
                if err then
                    mod_query.warn(mplex, err)
                end
            elseif e.submenu_fn then
                mod_query.query_menu(mplex, e.submenu_fn(),
                                     TR("%s:", e.name))
            end
        else
            mod_query.warn(mplex, TR("No entry '%s'", str))
        end
    end
    
    mod_query.query(mplex, prompt, nil, handle, 
                    mod_query.make_completor(complete), "menu")
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
        local i=cwin:get_ident()
        local s=TR("Title: %s\nClass: %s\nRole: %s\nInstance: %s\nXID: 0x%x",
                   n(cwin:name()), n(i.class), n(i.role), n(i.instance), 
                   cwin:xid())
        local t=TR("\nTransients:\n")
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

-- Load extras
dopath('mod_query_chdir')

-- Mark ourselves loaded.
_LOADED["mod_query"]=true


-- Load configuration file
dopath('cfg_query', true)
