--[[
Author: Etan Reisner
Email: deryni@unreliablesource.net
Summary: Maintains the _NET_CLIENT_LIST property (and the _NET_CLIENT_LIST_STACKING property incorrectly) on the root window.
Last Updated: 2007-07-22

Copyright (c) Etan Reisner 2007

This software is released under the terms of the MIT license. For more
information, see http://opensource.org/licenses/mit-license.php .
--]]

local atom_window = ioncore.x_intern_atom("WINDOW", false)
local atom_client_list = ioncore.x_intern_atom("_NET_CLIENT_LIST", false)
local atom_client_list_stacking = ioncore.x_intern_atom("_NET_CLIENT_LIST_STACKING", false)

local function add_client(cwin)
        if not cwin then
                return
        end

        local rootwin = cwin:rootwin_of()
        local list = {n=0}

        ioncore.clientwin_i(function (cwin)
                list.n = list.n + 1
                list[list.n] = cwin:xid()
                return true
        end)
        list.n = nil

        ioncore.x_change_property(rootwin:xid(), atom_client_list, atom_window,
                                  32, "replace", list)
        ioncore.x_change_property(rootwin:xid(), atom_client_list_stacking,
                                  atom_window, 32, "replace", list)
end

local function remove_client(xid)
        local rootwin = ioncore.current():rootwin_of()
        local list = {n=0}

        ioncore.clientwin_i(function (cwin)
                list.n = list.n + 1
                list[list.n] = cwin:xid()
                return true
        end)
        list.n = nil

        ioncore.x_change_property(rootwin:xid(), atom_client_list, atom_window,
                                  32, "replace", list)
        ioncore.x_change_property(rootwin:xid(), atom_client_list_stacking,
                                  atom_window, 32, "replace", list)
end

local function net_mark_supported(atom)
        if (ioncore.rootwin) then
                local rootwin = ioncore.rootwin()
                local atom_atom = ioncore.x_intern_atom("ATOM", false)
                local atom_net_supported = ioncore.x_intern_atom("_NET_SUPPORTED", false)
                ioncore.x_change_property(rootwin:xid(), atom_net_supported, atom_atom,
                                  32, "append", {atom})
        end
end

add_client(ioncore.current())

do
        local hook

        hook = ioncore.get_hook("clientwin_mapped_hook")
        if hook then
                hook:add(add_client)
        end
        hook = nil
        hook = ioncore.get_hook("clientwin_unmapped_hook")
        if hook then
                hook:add(remove_client)
        end

        net_mark_supported(atom_client_list);
end
