--
-- ion/share/ioncore-wd.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local savefile="saved_wd"
local dirs={}

local function regtreepath_i(reg)
    local function f(s, v)
        if v then
            return v:manager()
        else
            return s
        end
    end
    return f, reg, nil
end

--DOC
-- Change default working directory for new programs started in \var{reg}.
function ioncore.chdir_for(reg, dir)
    assert(type(dir)=="string")
    if dir=="" then
        dirs[reg]=nil
    else
        dirs[reg]=dir
    end
end

--DOC
-- Get default working directory for new programs started in \var{reg}.
function ioncore.get_dir_for(reg)
    for r in regtreepath_i(reg) do
        if dirs[r] then
            return dirs[r]
        end
    end
end

--DOC
-- Run \var{cmd} with the environment variable DISPLAY set to point to the
-- root window of the X screen \var{reg} is on.
function ioncore.exec_on(reg, cmd)
    return ioncore.do_exec_rw(reg:rootwin_of(), cmd, ioncore.get_dir_for(reg))
end


local function load_config()
    local d=ioncore.read_savefile(savefile)
    if d then
        dirs={}
        for nm, d in d do
            local r=ioncore.lookup_region(nm)
            if r then
                dirs[r]=d
            end
        end
    end
end


local function save_config()
    local t={}
    for r, d in dirs do
        t[r:name()]=d
    end
    ioncore.write_savefile(savefile, t)
end


local function init()
    load_config()
    ioncore.get_hook("ioncore_snapshot_hook"):add(save_config)
    ioncore.get_hook("ioncore_post_layout_setup_hook"):add(load_config)
end

init()

