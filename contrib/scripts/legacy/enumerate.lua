-- Authors: Sadrul Habib Chowdhury <imadil@gmail.com>, René van Bevern <rvb@pro-linux.de>
-- License: Public domain
-- Last Changed: Unknown
--
-- enumerate.lua: prepends "X. " in front of the title of a client-window so
--                that it's easier to quickly switch between windows using
--                mod1+n
--
-- Notes: May require the latest release
--
-- Author: Sadrul Habib Chowdhury (Adil)
-- slightly modified by René van Bevern <rvb@pro-linux.de>

local hook = nil

enumerate = {}

function enumerate.update_client(win)
    local frm = win:manager()
    if not obj_is(frm, "WFrame") then return end
    local name = win:name()
    -- need to detect whether name already has an "X. " at the beginning
    -- if there is one, get rid of it
    s, __, n = string.find(name, "%d+%. (.*)")
    if s == 1 then
        name = n
    end

    name = (frm:get_index(win) + 1) .. ". " .. name
    print("setting name: " .. name)
    win:set_name(name)
end

function enumerate.update()
    local list = ioncore.clientwin_list()
    if not list then return end
    for _, v in pairs(list) do
        enumerate.update_client(v)
    end
end

function enumerate.defer(reg, ev)
    if obj_is(reg, "WClientWin") then
        if ev then
            local atom = ioncore.x_get_atom_name("", ev)
            if atom ~= "_NET_WM_NAME" and atom ~= "WM_NAME" then
                return
            end
            ioncore.defer(enumerate.update_client(reg))
            return
        end
        ioncore.defer(enumerate.update)
    end
end

function enumerate.init()
    local events = {
                    "region_activated_hook",
                    "clientwin_property_change_hook",
                }
    for _, name in pairs(events) do
        hook = ioncore.get_hook(name)
        if hook then
            hook:add(enumerate.defer)
        end
    end

    enumerate.update()
end

enumerate.init()

