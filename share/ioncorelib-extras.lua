--
-- ion/share/ioncorelib-extras.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local ioncorelib=_G.ioncorelib


function ioncorelib.propagate_close(reg, sub)
    if sub then reg=sub end

    if obj_is(reg, "WClientWin") then
        local l=reg:managed_list()
        if l then
            local r2=l[table.getn(l)]
            if r2 then
                return propagate_close(r2)
            end
        end
    end
    reg:close()
end                               

--DOC
-- Move currently viewed object left within the multiplexer.
function WMPlex.inc_managed_index(mplex, r)
    if not r then
        r=mplex:current()
    end
    if r then
        mplex:set_mgd_index(r, mplex:get_mgd_index(r)+1) 
    end
end

--DOC
-- Move currently viewed object right within the multiplexer.
function WMPlex.dec_managed_index(mplex, r)
    if not r then
        r=mplex:current()
    end
    if r then
        mplex:set_mgd_index(r, mplex:get_mgd_index(r)-1) 
    end
end


--DOC
-- Create new workspace on screen.
function ioncorelib.create_new_ws(scr, ws_type)
    scr:attach_new({
        type=(ws_type or DEFAULT_WS_TYPE),
        switchto=true
    })
end

--DOC
-- Close current workspace.
function ioncorelib.close_current_ws(scr)
    local c=scr:current()
    if obj_is(c, "WGenWS") then 
        c:close() 
    end
end


--DOC
-- Find an object with type name \var{t} managing \var{obj} or one of
-- its managers.
function ioncorelib.find_manager(obj, t)
    while obj~=nil do
        if obj_is(obj, t) then
            return obj
        end
        obj=obj:manager()
    end
end

--DOC
-- Export a list of functions from \var{lib} into global namespace.
function ioncorelib.export(lib, ...)
    for k, v in arg do
        _G[v]=lib[v]
    end
end

--DOC
-- Show manual page for \var{for_what}, ion if unset.
function ioncorelib.show_manual(for_what)
    local script=ioncore.lookup_script("ion-man")
    if script then
        ioncore.exec(script.." "..(for_what or "ion"))
    end
end

