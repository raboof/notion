--
-- ion/share/ext_misc.lua
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
-- the user with require/includer differences.
if _LOADED["ext_misc"] then return end

local ext_misc={}
_G["ext_misc"]=ext_misc


--DOC
-- Create new workspace on screen \var{scr}. If \var{ws_type} is
-- set, then a workspace of that type is created. Otherwise
-- a workspace of type indicated by the \code{DEFAULT_WS_TYPE}
-- variable is created.
function ext_misc.create_new_ws(scr, ws_type)
    scr:attach_new({
        type=(ws_type or DEFAULT_WS_TYPE),
        switchto=true
    })
end

--DOC
-- Close current workspace on screen \var{scr}.
function ext_misc.close_current_ws(scr)
    local c=scr:current()
    if obj_is(c, "WGenWS") then 
        c:close() 
    end
end


--DOC
-- Find an object with type name \var{t} managing \var{obj} or one of
-- its managers.
function ext_misc.find_manager(obj, t)
    while obj~=nil do
        if obj_is(obj, t) then
            return obj
        end
        obj=obj:manager()
    end
end


--DOC
-- Show manual page for \var{for_what}, ion if unset.
function ext_misc.show_manual(for_what)
    local script=ioncore.lookup_script("ion-man")
    if script then
        ioncore.exec(script.." "..(for_what or "ion"))
    end
end

-- Mark ourselves loaded.
_LOADED["ext_misc"]=true

