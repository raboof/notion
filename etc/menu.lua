--
-- Menu bindings
--


menu_bindings{
    kpress("Escape", WMenu.cancel),
    kpress("Control+G", WMenu.cancel),
    kpress("Control+C", WMenu.cancel),
    kpress("Control+P", WMenu.select_prev),
    kpress("Control+N", WMenu.select_next),
    kpress("Up", WMenu.select_prev),
    kpress("Down", WMenu.select_next),
    kpress("Return", WMenu.finish),
    kpress("KP_Enter", WMenu.finish),
    kpress("Control+M", WMenu.finish),
}

include('menulib.lua')

about_msg=[[
Ion  ]]..ioncore_version()..[[ , copyright (c) Tuomo Valkonen 1999-2003.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
Lesser General Public License for more details.]]

local function about(mplex)
    query_message(mplex, about_msg)
end

local appmenu={
    menuentry("XTerm", make_exec_fn("xterm")),
    menuentry("Mozilla Firebird", make_exec_fn("MozillaFirebird")),
    menuentry("Xdvi", make_exec_fn("xdvi")),
    menuentry("GV", make_exec_fn("gv")),
}

local mainmenu={
    submenu("Programs", appmenu),
    menuentry("Help", querylib.query_man),
    menuentry("About Ion", about),
    menuentry("Restart", querylib.query_restart),
    menuentry("Exit", querylib.query_exit),
}

local function make_tab_fn(fn)
    return function(frame, cwin)
               if cwin then fn(cwin) end
           end
end

local tabmenu={
    menuentry("Close", make_tab_fn(WRegion.close)),
    menuentry("Kill", make_tab_fn(WClientWin.kill)),
    menuentry("(Un)tag", make_tab_fn(WRegion.toggle_tag)),
    menuentry("(Un)shade", function(frame) frame:toggle_shade() end),
}


genframe_bindings{
    mpress("Button3", make_pmenu_fn(tabmenu), "tab"),
}

global_bindings{
    -- Rebind F12
    kpress("F12", make_menu_fn(mainmenu)),
    -- kpress("Menu", make_menu_fn(mainmenu)),
}

