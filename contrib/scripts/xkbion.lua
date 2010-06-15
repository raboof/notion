-- xkbion.lua
-- TODO: make xkbion_set understand some simple presets

-- (c) Sergey Redin <sergey at redin.info>
-- Thanks:
-- smersh at users.sf.net (author of xkbind) for the original idea

--[[

-- This script allows you to use independent keyboard layouts for different windows in Ion3.
-- It uses a window property to store the XKB groups, so you can restart Ion without losing
-- settings for each window.

-- Example usage. This is what I have in my cfg_ion.lua:

dopath("mod_xkb")
dopath("xkbion.lua")
xkbion_set {
    {name="EN", hint="", action = function() mod_xkb.lock_group(0) end},
    {name="RU", hint="important", action = function() mod_xkb.lock_group(1) end},
    key="Caps_Lock",
    statusname = "xkbion",
}
xkbion_set {
    {name="num", command="numlockx on"},
    {name="<->", command="numlockx off"},
    key="Num_Lock",
    statusname="num",
    atomname="XKBION_NUM",
}
xkbion_set {
    {name="----", hint="", action = function() mod_xkb.lock_modifiers(2, 0) end},
    {name="CAPS", hint="critical", action = function() mod_xkb.lock_modifiers(2, 2) end},
    key="Caps_Lock",
    statusname = "caps",
    atomname="XKBION_CAPS",
}

-- Edit this to suit your needs.
-- Please note, if you want to use Caps_Lock key to change the keyboard groups like I do,
-- do not forget to add "grp:caps_toggle" to your XKB settings, just to prevent X from using
-- this key also for swiching keyboard registers.

-- At least one group definition must be present.
-- "name" is only neseccary if you want to use mod_statusbar to indicate current XKB group.
-- "hint" is only necessary if you want to highlight your XKB group in statusbar, possible
-- values are standard values provided by the mod_statusbar: important, normal, critical
-- "command" and "action" are also unneseccary but xkbion.lua is not particulary useful
-- without them. :) The same thing for "key".

-- The last thing to say about xkbion_set() parameters is that if you call xkbion_set
-- more than once (like I do it for XKB groups and NumLock state) you must choose different
-- "atomname" values. The default for atomname is XKBION_GROUP.

-- The second xkbion_set() call (numlock section) is here mostly for the example. Most users
-- will need only one call, for changing XKB group. Please also note that you can define more
-- than two groups in call to xkbion_set().

-- You can use this line in cfg_statusbar.lua to indicate the current XKB group:

template="... %xkbion ...",

-- If your Ion does not have mod_xkb, you may try the following:

xkbion_set {
    {name="EN", command="setxkbmap us -option grp:caps_toggle"},
    {name="RU", command="setxkbmap ru winkeys -option grp:caps_toggle"},
    key="Caps_Lock",
    statusname = "xkbion",
}

]]

function xkbion_set (groups) -- the only global created by xkbion.lua

    if not groups or type(groups) ~= "table" then error("bad args") end
    if not groups[1] or type(groups[1]) ~= "table" then
        error("default group is undefined")
    end

    -- window_group_prop(w) - get XKBION_GROUP integer property of window `w' (set it to 1 if it's not yet defined)
    -- window_group_prop(w, group) - set XKBION_GROUP property of window `w' to integer `group'
    -- "XKBION_GROUP" is just the default name
    local window_group_prop
    do
        local XA_INTEGER = 19
        local atom = ioncore.x_intern_atom( tostring( groups.atomname or "XKBION_GROUP" ) )
        if not atom or type(atom) ~= "number" then
            error("Cannot intern atom " .. atomname)
        end
        window_group_prop = function(w, gnum)
            if not w or type(w) ~= "userdata" or not w.xid or type(w.xid) ~= "function" then return 1 end
            local xid = tonumber( w:xid() )
            if gnum == nil then
                local t = ioncore.x_get_window_property( xid, atom, XA_INTEGER, 1, true )
                if t and type(t) == "table" and t[1] ~= nil then
                    do return tonumber(t[1]) end
                else
                    gnum = 1
                end
            else
                gnum = tonumber(gnum)
            end
            -- we're here if the second argument is set or if the window does not have our property yet
            ioncore.defer( function()
                ioncore.x_change_property( xid, atom, XA_INTEGER, 32, "replace", {gnum} )
            end )
            return gnum
        end
    end
    
    local set_group
    do
        local current_gnum = 1
        local first_time = true
        local statusname = groups.statusname
        if statusname and type(statusname) ~= "string" then statusname = nil end
        set_group = function(w, do_increment)
            local gnum
            if w then
                gnum = window_group_prop(w)
            else
                gnum = 1
            end
            if do_increment then gnum = gnum + 1 end
            local g = groups[gnum]
            if not g then gnum, g = 1, groups[1] end
            if not g then return end -- error in settings, groups[1] not defined
            if first_time then
                first_time = false
            elseif gnum == current_gnum then
                return
            end
            window_group_prop(w, gnum) -- it's OK to call it even it `w' is nil
            if g.command then
                ioncore.exec(g.command)
            end
            if g.action then ioncore.defer(g.action) end
            current_gnum = gnum
            local group_name = g.name
            local hint_name = g.hint
            if statusname and group_name and type(group_name) == "string" then
                mod_statusbar.inform(statusname, group_name)
                mod_statusbar.inform(statusname.."_hint", hint_name)
                ioncore.defer(mod_statusbar.update)
            end
        end
    end
    
    ioncore.get_hook("region_notify_hook"):add(
        function(reg, action)
            if (obj_typename(reg) == "WClientWin") and (action == "activated") then
                set_group(reg)
            end
        end
    )

    local key = groups.key
    if key and type(key) == "string" then
        defbindings("WClientWin", {
            kpress(key, function (_, _sub) set_group(_, true)  end, "_sub:WClientWin")
        })
    end

    set_group() -- initialize

end -- xkbion_set()
