--
-- ion/query/mod_query_chdir.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2009.
-- 
-- See the included file LICENSE for details.
--

local function simplify_path(path)
    local npath=string.gsub(path, "([^/]+)/+%.%./+", "")
    if npath~=path then
        return simplify_path(npath)
    else
        return string.gsub(string.gsub(path, "([^/]+)/+%.%.$", ""), "/+", "/")
    end
end

local function relative_path(path)
    return not string.find(path, "^/")
end

local function empty_path(path)
    return (not path or path=="")
end

local function ws_chdir(mplex, params)
    local nwd=params[1]
    
    ws=assert(ioncore.find_manager(mplex, "WGroupWS"))
    
    if not empty_path(nwd) and relative_path(nwd) then
        local owd=ioncore.get_dir_for(ws)
        if empty_path(owd) then
            owd=os.getenv("PWD")
        end
        if owd then
            nwd=owd.."/"..nwd
        end
    end
    local ok, err=ioncore.chdir_for(ws, nwd and simplify_path(nwd))
    if not ok then
        mod_query.warn(mplex, err)
    end
end

local function ws_showdir(mplex, params)
    local dir=ioncore.get_dir_for(mplex)
    if empty_path(dir) then
        dir=os.getenv("PWD")
    end
    mod_query.message(mplex, dir or "(?)")
end

mod_query.defcmd("cd", ws_chdir)
mod_query.defcmd("pwd", ws_showdir)
