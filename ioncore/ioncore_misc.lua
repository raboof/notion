--
-- ion/share/ioncore_misc.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2007.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local group_tmpl={type="WGroupWS", switchto=true}

local empty = { managed={} }

local layouts={
    empty = empty,
    default = empty,
}

--DOC 
-- Define a new workspace layout with name \var{name}, and
-- attach/creation parameters given in \var{tab}. The layout
-- "empty" may not be defined.
function ioncore.deflayout(name, tab)
    assert(layout ~= "empty")

    if name=="default" and not tab then
        tab = empty
    end

    layouts[name] = tab
end

--DOC
-- Get named layout (or all of the latter parameter is set,
-- but this is for internal use only).
function ioncore.getlayout(name, all)
    if all then
        return layouts
    else
        return layouts[name]
    end
end

ioncore.set{_get_layout=ioncore.getlayout}

--DOC
-- Create new workspace on screen \var{scr}. The table \var{tmpl}
-- may be used to override parts of the layout named with \code{layout}.
-- If no \var{layout} is given, "default" is used.
function ioncore.create_ws(scr, tmpl, layout)
    local lo=ioncore.getlayout(layout or "default")
    local t=table.join(table.join(tmpl or {}, lo), group_tmpl)
    
    return scr:attach_new(t)
end


--DOC
-- Find an object with type name \var{t} managing \var{obj} or one of
-- its managers.
function ioncore.find_manager(obj, t)
    while obj~=nil do
        if obj_is(obj, t) then
            return obj
        end
        obj=obj:manager()
    end
end


--DOC
-- gettext+string.format
function ioncore.TR(s, ...)
    return string.format(ioncore.gettext(s), unpack(arg))
end


--[[DOC
-- Run \var{cmd} with the environment variable DISPLAY set to point to the
-- root window of the X screen \var{reg} is on.
function ioncore.exec_on(reg, cmd)
    return ioncore.do_exec_rw(reg:rootwin_of(), cmd)
end
--]]
