--
-- ion/query/mod_query.lua -- Some common queries for Ion
--
-- Copyright (c) Tuomo Valkonen 2004-2007.
--
-- See the included file LICENSE for details.
--


-- This is a slight abuse of the package.loaded variable perhaps, but
-- library-like packages should handle checking if they're loaded instead of
-- confusing the user with require/include differences.
if package.loaded["mod_query"] then return end

if not ioncore.load_module("mod_query") then
    return
end

local mod_query=_G["mod_query"]
assert(mod_query)

local loadstring = loadstring or load

local DIE_TIMEOUT_ERRORCODE=10 -- 10 seconds
local DIE_TIMEOUT_NO_ERRORCODE=2 -- 2 seconds


-- Generic helper functions {{{


--DOC
-- Display an error message box in the multiplexer \var{mplex}.
function mod_query.warn(mplex, str)
    ioncore.unsqueeze(mod_query.do_warn(mplex, str))
end


--DOC
-- Display a message in \var{mplex}.
function mod_query.message(mplex, str)
    ioncore.unsqueeze(mod_query.do_message(mplex, str))
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
    local function cycle(wedln)
        wedln:complete('next', 'normal')
    end
    local function bcycle(wedln)
        wedln:complete('prev', 'normal')
    end

    -- Check that no other queries are open in the mplex.
    local ok=mplex:managed_i(function(r)
                                 return not obj_is(r, "WEdln")
                             end)
    if not ok then
        return
    end

    local wedln=mod_query.do_query(mplex, prompt, initvalue,
                                   handle_it, completor, cycle, bcycle)

    if wedln then
        ioncore.unsqueeze(wedln)

        if context then
            wedln:set_context(context)
        end
    end

    return wedln
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
for _, v in pairs(badsig_) do
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
                                   ic.." "..string.shell_safe(str))
    end
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


-- }}}


-- Completion helpers {{{

local pipes={}

mod_query.COLLECT_THRESHOLD=2000

--DOC
-- This function can be used to read completions from an external source.
-- The parameter \var{cp} is the completion proxy to be used,
-- and the string \var{cmd} the shell command to be executed. To its stdout,
-- the command should on the first line write the \var{common_beg}
-- parameter of \fnref{WComplProxy.set_completions} (which \var{fn} maybe used
-- to override) and a single actual completion on each of the successive lines.
-- The function \var{reshnd} may be used to override a result table
-- building routine.
function mod_query.popen_completions(cp, cmd, fn, reshnd)

    local pst={cp=cp, maybe_stalled=0}

    if not reshnd then
        reshnd = function(rs, a)
                     if not rs.common_beg then
                         rs.common_beg=a
                     else
                         table.insert(rs, a)
                     end
                 end
    end

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
                             function(s)
                                 reshnd(results, s)
                                 lines=lines+1
                                 return ""
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

    for k, v in pairs(pipes) do
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


local function mk_completion_test(str, sub_ok, casei_ok)
    local settings=mod_query.get()

    if not str then
        return function(s) return true end
    end

    local function mk(str, sub_ok)
        if sub_ok then
            return function(s) return string.find(s, str, 1, true) end
        else
            local len=string.len(str)
            return function(s) return string.sub(s, 1, len)==str end
        end
    end

    local casei=(casei_ok and mod_query.get().caseicompl)

    if not casei then
        return mk(str, sub_ok)
    else
        local fn=mk(string.lower(str), sub_ok)
        return function(s) return fn(string.lower(s)) end
    end
end


local function mk_completion_add(entries, str, sub_ok, casei_ok)
    local tst=mk_completion_test(str, sub_ok, casei_ok)

    return function(s)
               if s and tst(s) then
                   table.insert(entries, s)
               end
           end
end


function mod_query.complete_keys(list, str, sub_ok, casei_ok)
    local results={}
    local test_add=mk_completion_add(results, str, sub_ok, casei_ok)

    for m, _ in pairs(list) do
        test_add(m)
    end

    return results
end


function mod_query.complete_name(str, iter)
    local sub_ok_first=true
    local casei_ok=true
    local entries={}
    local tst_add=mk_completion_add(entries, str, sub_ok_first, casei_ok)

    iter(function(reg)
             tst_add(reg:name())
             return true
         end)

    if #entries==0 and not sub_ok_first then
        local tst_add2=mk_completion_add(entries, str, true, casei_ok)
        iter(function(reg)
                 tst_add2(reg:name())
                 return true
             end)
    end

    return entries
end


function mod_query.make_completor(completefn)
    local function completor(cp, str, point)
        cp:set_completions(completefn(str, point))
    end
    return completor
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


function mod_query.complete_clientwin(str)
    return mod_query.complete_name(str, ioncore.clientwin_i)
end


function mod_query.complete_workspace(str)
    local function iter(fn)
        return ioncore.region_i(function(obj)
                                    return (not obj_is(obj, "WGroupWS")
                                            or fn(obj))
                                end)
    end
    return mod_query.complete_name(str, iter)
end


function mod_query.complete_region(str)
    return mod_query.complete_name(str, ioncore.region_i)
end


function mod_query.gotoclient_handler(frame, str)
    local cwin=ioncore.lookup_clientwin(str)

    if cwin==nil then
        mod_query.warn(frame, TR("Could not find client window %s.", str))
    else
        cwin:goto_focus()
    end
end


function mod_query.attachclient_handler(frame, str)
    local cwin=ioncore.lookup_clientwin(str)

    if not cwin then
        mod_query.warn(frame, TR("Could not find client window %s.", str))
        return
    end

    local reg=cwin:groupleader_of()

    local function attach()
        frame:attach(reg, { switchto = true })
    end

    if frame:rootwin_of()~=reg:rootwin_of() then
        mod_query.warn(frame, TR("Cannot attach: different root windows."))
    elseif reg:manager()==frame then
        reg:goto_focus()
    else
        mod_query.call_warn(frame, attach)
    end
end


function mod_query.workspace_handler(mplex, name)
    local ws=ioncore.lookup_region(name, "WGroupWS")
    if ws then
        ws:goto_focus()
    else
        local function create_handler(mplex_, layout)
            if not layout or layout=="" then
                layout="default"
            end

            if not ioncore.getlayout(layout) then
                mod_query.warn(mplex_, TR("Unknown layout"))
            else
                local scr=mplex:screen_of()

                local function mkws()
                    local tmpl={name=name, switchto=true}
                    if not ioncore.create_ws(scr, tmpl, layout) then
                        error(TR("Unknown error"))
                    end
                end

                mod_query.call_warn(mplex, mkws)
            end
        end

        local function compl_layout(str)
            local los=ioncore.getlayout(nil, true)
            return mod_query.complete_keys(los, str, true, true)
        end

        mod_query.query(mplex, TR("New workspace layout (default):"), nil,
                        create_handler, mod_query.make_completor(compl_layout),
                        "workspacelayout")
    end
end


--DOC
-- This query asks for the name of a client window and switches
-- focus to the one entered. It uses the completion function
-- \fnref{ioncore.complete_clientwin}.
function mod_query.query_gotoclient(mplex)
    mod_query.query(mplex, TR("Go to window:"), nil,
                    mod_query.gotoclient_handler,
                    mod_query.make_completor(mod_query.complete_clientwin),
                    "windowname")
end

--DOC
-- This query asks for the name of a client window and attaches
-- it to the frame the query was opened in. It uses the completion
-- function \fnref{ioncore.complete_clientwin}.
function mod_query.query_attachclient(mplex)
    mod_query.query(mplex, TR("Attach window:"), nil,
                    mod_query.attachclient_handler,
                    mod_query.make_completor(mod_query.complete_clientwin),
                    "windowname")
end


--DOC
-- This query asks for the name of a workspace. If a workspace
-- (an object inheriting \type{WGroupWS}) with such a name exists,
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
    mod_query.query_yesno(mplex, TR("Exit Notion/Shutdown session (y/n)?"),
                         ioncore.shutdown)
end


--DOC
-- This query asks whether the user wants restart Ioncore.
-- If the answer is 'y', 'Y' or 'yes', so will happen.
function mod_query.query_restart(mplex)
    mod_query.query_yesno(mplex, TR("Restart Notion (y/n)?"), ioncore.restart)
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
-- This function asks for a name new for the workspace \var{ws},
-- or the one on which \var{mplex} resides, if it is not set.
-- If \var{mplex} is not set, one is looked for.
function mod_query.query_renameworkspace(mplex, ws)
    if not mplex then
        assert(ws)
        mplex=ioncore.find_manager(ws, "WMPlex")
    elseif not ws then
        assert(mplex)
        ws=ioncore.find_manager(mplex, "WGroupWS")
    end

    assert(mplex and ws)

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
        local n=#res
        if string.find(res[n], "^%s+$") then
            table.insert(res, str)
        else
            res[n]=res[n]..str
        end
    end

    local function ins_space(str)
        local n=#res
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
    return #strs
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

    for i=complidx+1, #parts do
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

    local function filter_fn(res, s)
        if not res.common_beg then
            if s=="./" then
                res.common_beg=""
            else
                res.common_beg=s
            end
        else
            table.insert(res, s)
        end
    end

    local ic=ioncore.lookup_script("ion-completefile")
    if ic then
        mod_query.popen_completions(wedln,
                                   ic..wp..string.shell_safe(s_compl),
                                   set_fn, filter_fn)
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
mod_query.hostnicks={}
mod_query.ssh_completions={}


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
                for s in string.gmatch(patterns, "%S+") do
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
    local tst = mk_completion_test(host, true, false)

    for _, v in ipairs(mod_query.ssh_completions) do
        if tst(v) then
            table.insert(res, user .. v)
        end
    end

    return res
end


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
    local icase=(mod_query.get().caseicompl and " -icase" or "")
    local mid=""
    if mc then
        mod_query.popen_completions(wedln, (mc..icase..mid.." -complete "
                                            ..string.shell_safe(str)))
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
    local origenv
    if _ENV then origenv=_ENV else origenv=getfenv() end
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
        local arg={...}
        local l=#arg
        for i=1,l do
            tmp=tmp..tostring(arg[i])..(i==l and "\n" or "\t")
        end
        print_res=(print_res and print_res..tmp or tmp)
    end


    if _ENV then
        env.print=collect_print
        local f, err=load(code,nil, nil, env)
        if not f then
            mod_query.warn(mplex, err)
            return
        end
        err=collect_errors(f)
    else
        local f, err=loadstring(code)
        if not f then
            mod_query.warn(mplex, err)
            return
        end
        env.print=collect_print
        setfenv(f, env)
        err=collect_errors(f)
    end
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
        for t in string.gmatch(tocomp, "([^.:]*)[.:]") do
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
        if type(tab) == "table" then
            for k in pairs(tab) do
                if type(k)=="string" then
                    if string.sub(k, 1, l)==tocomp then
                        table.insert(compl, k)
                    end
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
    if #compl==1 then
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

    mod_query.query(mplex, TR("Lua code:"), nil, handler, complete, "lua")
end

-- }}}


-- Menu query {{{

--DOC
-- This query can be used to create a query of a defined menu.
function mod_query.query_menu(mplex, sub, themenu, prompt)
    if type(sub)=="string" then
        -- Backwards compat. shift
        prompt=themenu
        themenu=sub
        sub=nil
    end

    local menu=ioncore.evalmenu(themenu, mplex, sub)
    local menuname=(type(themenu)=="string" and themenu or "?")

    if not menu then
        mod_query.warn(mplex, TR("Unknown menu %s.", tostring(themenu)))
        return
    end

    if not prompt then
        prompt=menuname..":"
    else
        prompt=TR(prompt)
    end

    local function xform_name(n, is_submenu)
        return string.lower(string.gsub(n, "[-/%s]+", "-"))
    end

    local function xform_menu(t, m, p)
        for _, v in ipairs(m) do
            if v.name then
                local is_submenu=v.submenu_fn
                local n=p..xform_name(v.name)

                while t[n] or t[n..'/'] do
                    n=n.."'"
                end

                if is_submenu then
                    n=n..'/'
                end

                t[n]=v

                if is_submenu and not v.noautoexpand then
                    local sm=v.submenu_fn()
                    if sm then
                        xform_menu(t, sm, n)
                    else
                        ioncore.warn_traced(TR("Missing submenu ")
                                            ..(v.name or ""))
                    end
                end
            end
        end
        return t
    end

    local ntab=xform_menu({}, menu, "")

    local function complete(str)
        -- casei_ok false, because everything is already in lower case
        return mod_query.complete_keys(ntab, str, true, false)
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
                    mod_query.make_completor(complete), "menu."..menuname)
end

-- }}}


-- Miscellaneous {{{


--DOC
-- Display an "About Ion" message in \var{mplex}.
function mod_query.show_about_ion(mplex)
    mod_query.message(mplex, ioncore.aboutmsg())
end


--DOC
-- Show information about a region tree
function mod_query.show_tree(mplex, reg, max_depth)
    local function indent(s)
        local i="    "
        return i..string.gsub(s, "\n", "\n"..i)
    end

    local function get_info(reg, indent, d)
        if not reg then
            return (indent .. "No region")
        end

        local function n(s) return (s or "") end

        local s=string.format("%s%s \"%s\"", indent, obj_typename(reg),
                              n(reg:name()))
        indent = indent .. "  "
        if obj_is(reg, "WClientWin") then
            local i=reg:get_ident()
            s=s .. TR("\n%sClass: %s\n%sRole: %s\n%sInstance: %s\n%sXID: 0x%x",
                      indent, n(i.class),
                      indent, n(i.role),
                      indent, n(i.instance),
                      indent, reg:xid())
        end

        if (not max_depth or max_depth > d) and reg.managed_i then
            local first=true
            reg:managed_i(function(sub)
                              if first then
                                  s=s .. "\n" .. indent .. "---"
                                  first=false
                              end
                              s=s .. "\n" .. get_info(sub, indent, d+1)
                              return true
                          end)
        end

        return s
    end

    mod_query.message(mplex, get_info(reg, "", 0))
end

-- }}}

-- Load extras
dopath('mod_query_chdir')

-- Mark ourselves loaded.
package.loaded["mod_query"]=true


-- Load configuration file
dopath('cfg_query', true)
