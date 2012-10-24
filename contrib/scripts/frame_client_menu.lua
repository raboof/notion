--[[
Author: Etan Reisner
Email: deryni@gmail.com
Summary: Adds a submenu to the frame context menu which contains the cwins in that frame.
Version: 0.1
Last Updated: 2007-01-29

Copyright (c) Etan Reisner 2007

This software is released under the terms of the MIT license. For more
information, see http://opensource.org/licenses/mit-license.php .
--]]

-- Usage:
--        This will show the frame context query menu with the added submenu.
--          kpress(META.."M", "clientlist_menu(_, _sub)")
--        This will show the frame context popup menu with the added submenu.
--          kpress(META.."M", "clientlist_menu(_, _sub, true)")
--
-- Use frame_client_menu_set({start_on_current=true/false}) to control whether
-- the popup submenu will start on the current frame or index 1.

local settings = {start_on_current = false}

function frame_client_menu_set(tab)
    settings.start_on_current = tab.start_on_current
end

function frame_client_menu_get()
    return table.copy(settings)
end

function clientlist_menu(frame, sub, popup)
    local start_on_current = false

    if mod_menu then
        local framemenu = ioncore.getmenu("ctxmenu")(frame, sub)
        local myframemenu = {}
        local initial = 1

        frame:mx_i(function(reg)
                       if reg == sub then
                           initial = #myframemenu + 1
                       end
                       myframemenu[#myframemenu + 1] = ioncore.menuentry(reg:current():name(), function() reg:goto() end)

                       return true
                   end)

        if not settings.start_on_current then
            initial = 1
        end

        framemenu[#framemenu + 1] = ioncore.submenu("Client windows", myframemenu, {initial=initial})

        if popup then
            mod_menu.menu(frame, sub, framemenu)
        else
            mod_query.query_menu(frame, framemenu, "Context menu:")
        end
    end
end

-- vim: set expandtab sw=4:
