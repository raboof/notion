--
-- ion/query/mod_query_chdir.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2006.
-- 
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local function ws_chdir(mplex, params)
    ws=assert(ioncore.find_manager(mplex, "WGenWS"))
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
