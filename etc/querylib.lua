--
-- Some common queries for Ion
-- 

if QueryLib~=nil then
    return
end

QueryLib={}


--
-- Helper functions
-- 

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

function QueryLib.exec_handler(frame, cmd)
    if string.sub(cmd, 1, 1)==":" then
        cmd="ion-runinxterm " .. string.sub(cmd, 2)
    end
    exec_on_screen(region_screen_of(frame), cmd)
end

function QueryLib.make_yesno_handler(fn)
    local function handle_yesno(_, yesno)
        if yesno=="y" or yesno=="Y" or yesno=="yes" then
            if arg then
                fn(unpack(arg))
            else
                fn()
            end
        end
    end
    return handle_yesno
end

function QueryLib.make_yesno_fn(prompt, handler)
    return QueryLib.make_frame_fn(prompt, nil,
                                  QueryLib.make_yesno_handler(handler),
                                  nil)
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

function QueryLib.make_execwith_fn(prompt, init, prog, completor)
    local function handle_execwith(frame, str)
        exec_on_screen(region_screen_of(frame), prog .. " " .. str)
    end
    return QueryLib.make_frame_fn(prompt, init, handle_execwith, completor)
end

function QueryLib.gotoclient_handler(frame, str)
    local cwin=lookup_clientwin(str)
    
    if cwin==nil then
        query_fwarn(frame, string.format("Could not find client window named"
                                         .. ' "%s"', str))
    else
        region_goto(cwin)
    end
end

function QueryLib.get_initdir()
    local wd=os.getenv("PWD")
    if wd==nil then
        wd="/"
    elseif string.sub(wd, -1)~="/" then
        wd=wd .. "/"
    end
    return wd
end

function QueryLib.handler_lua(frame, code)
    local errors
    local oldarg=arg
    arg={frame, genframe_current(frame)}
    errors=collect_errors(function()
                              f=loadstring(code)
                              f()
			  end)
    arg=oldarg;
    if errors then
        query_fwarn(frame, errors)
    end
end

function QueryLib.complete_function(str)
    local res={}
    local len=string.len(str)
    for k, v in pairs(_G) do
        if type(v)=="function" and string.sub(k, 1, len)==str then
            table.insert(res, k)
        end
    end
    return res
end

-- Use weak references to cache found manuals.
QueryLib.mancache={__mode="v"}

function QueryLib.complete_man(str)
    local function find_manuals()
        local manuals={}
        -- Maybe we should attempt to grep /etc/manpath.config and
        -- check the MANPATH environment variable?
        local dirs=table.concat(query_man_path, " ")
        if dirs==nil then
            return {}
        end
        local h=io.popen("find "..dirs.." -type f", "r");
        for l in h:lines() do
            -- Extract page name part and try to ignore non-manpage files
            for a in string.gfind(l, ".*/([^/.]+)%.%d.*") do
                table.insert(manuals, a)
            end
        end
        h:close()
        return manuals
    end
    
    manuals=QueryLib.mancache.manuals
    if not manuals then
        -- Manuals were not cached, find them
        manuals=find_manuals()
        QueryLib.mancache.manuals=manuals
    end

    local results={}
    local len=string.len(str)
    if len==0 then
        return manuals
    end
    
    for _, m in manuals do
        if string.sub(m, 1, len)==str then
            table.insert(results, m)
        end
    end
    return results
end

--    
-- The queries
-- 

QueryLib.query_gotoclient=QueryLib.make_frame_fn("Go to window:", nil,
                                                 QueryLib.gotoclient_handler,
                                                 complete_clientwin)

QueryLib.query_attachclient=QueryLib.make_frame_fn("Attach window:", nil,
                                                    query_handler_attachclient,
                                                    complete_clientwin)

QueryLib.query_workspace=QueryLib.make_frame_fn("Go to or create workspace:",
                                                nil, query_handler_workspace,
                                                complete_workspace)

QueryLib.query_exec=QueryLib.make_frame_fn("Run:", nil,
                                           QueryLib.exec_handler,
                                           complete_file_with_path)    

QueryLib.query_exit=QueryLib.make_yesno_fn("Exit Ion (y/n)?",
                                           exit_wm)

QueryLib.query_restart=QueryLib.make_yesno_fn("Restart Ion (y/n)?",
                                              restart_wm)

QueryLib.query_renameframe=QueryLib.make_rename_fn("Frame name: ",
                                                   function(frame)
                                                       return frame
                                                   end
                                                  )

QueryLib.query_renameworkspace=QueryLib.make_rename_fn("Workspace name: ",
                                                       QueryLib.getws)

QueryLib.query_ssh=QueryLib.make_execwith_fn("SSH to:", nil, "ion-ssh",
                                             QueryLib.complete_ssh)

QueryLib.query_man=QueryLib.make_execwith_fn("Manual page (ion):",
                                             nil,
                                             "ion-man",
                                             QueryLib.complete_man)

QueryLib.query_editfile=QueryLib.make_execwith_fn("Edit file:",
                                                  QueryLib.get_initdir,
                                                  "ion-edit",
                                                  complete_file)

QueryLib.query_runfile=QueryLib.make_execwith_fn("View file:",
                                                 QueryLib.get_initdir,
                                                 "ion-view",
                                                 complete_file)

QueryLib.query_lua=QueryLib.make_frame_fn("Lua code to run:",
                                          nil,
                                          QueryLib.handler_lua,
                                          QueryLib.complete_function);
