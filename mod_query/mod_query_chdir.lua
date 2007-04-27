--
-- ion/query/mod_query_chdir.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2007.
-- 
-- See the included file LICENSE for details.
--

local function ws_chdir(mplex, params)
    ws=assert(ioncore.find_manager(mplex, "WGroupWS"))
    local ok, err=ioncore.chdir_for(ws, params[1] or "")
    if not ok then
        mod_query.warn(mplex, err)
    end
end

local function ws_showdir(mplex, params)
    local dir=assert(ioncore.get_dir_for(mplex) or os.getenv("PWD"))
    mod_query.message(mplex, dir)
end

mod_query.defcmd("cd", ws_chdir)
mod_query.defcmd("pwd", ws_showdir)
