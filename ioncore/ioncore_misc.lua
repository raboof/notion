--
-- ion/share/ioncore_misc.lua
--
-- Copyright (c) Tuomo Valkonen 2004-2007.
--
-- See the included file LICENSE for details.
--

local group_tmpl = { type="WGroupWS" }

local default_tmpl = { switchto=true }

local empty = { type="WGroupWS", managed={} }

local layouts={
    empty = empty,
    default = empty,
}

--DOC
-- Define a new workspace layout with name \var{name}, and
-- attach/creation parameters given in \var{tab}. The layout
-- "empty" may not be defined.
function ioncore.deflayout(name, tab)
    assert(name ~= "empty")

    if name=="default" and not tab then
        layouts[name] = empty
    else
        layouts[name] = table.join(tab, group_tmpl)
    end
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

    assert(lo, TR("Unknown layout"))

    return scr:attach_new(table.join(tmpl or default_tmpl, lo))
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
    local arg = {...}
    return string.format(ioncore.gettext(s), unpack(arg))
end


--DOC
-- Attach tagged regions to \var{reg}. The method of attach
-- depends on the types of attached regions and whether \var{reg}
-- implements \code{attach_framed} and \code{attach}. If \var{param}
-- is not set, the default of \verb!{switchto=true}! is used.
function ioncore.tagged_attach(reg, param)
    if not param then
        param={switchto=true}
    end
    local tagged=function() return ioncore.tagged_first(true) end
    for r in tagged do
        local fn=((not obj_is(r, "WWindow") and reg.attach_framed)
                  or reg.attach)

        if not (fn and fn(reg, r, param)) then
            return false
        end
    end
    return true
end

