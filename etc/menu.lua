--
-- Menu bindings
--

-- The menus

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

local exitmenu={
    menuentry("Exit", querylib.query_exit),
    menuentry("Restart", querylib.query_restart),
    menuentry("Restart PWM", function() restart_other_wm("pwm") end),
    menuentry("Restart TWM", function() restart_other_wm("twm") end),
}

local mainmenu={
    submenu("Programs", appmenu),
    menuentry("Lock screen", make_exec_fn("xlock")),
    menuentry("Help", querylib.query_man),
    menuentry("About Ion", about),
    submenu("Exit", exitmenu),
}
    
local ctxmenu={
    menuentry("Close", make_menu_ctx_fn(WRegion.close)),
    menuentry("Kill", make_menu_ctx_fn(WClientWin.kill)),
    menuentry("(Un)tag", make_menu_ctx_fn(WRegion.toggle_tag)),
    menuentry("Attach tagged", WGenFrame.attach_tagged),
}


-- Bindings to display menus

genframe_bindings{
    mpress("Button3", make_pmenu_fn(ctxmenu), "tab"),
    kpress(DEFAULT_MOD.."M", make_menu_fn(ctxmenu)),
    kpress(DEFAULT_MOD.."Menu", make_menu_fn(ctxmenu)),
}

global_bindings{
    -- Rebind F12
    kpress("F12", make_bigmenu_fn(mainmenu)),
    kpress("Menu", make_bigmenu_fn(mainmenu)),
}


-- Menu bindings

menu_bindings{
    kpress("Escape", WMenu.cancel),
    kpress("Control+G", WMenu.cancel),
    kpress("Control+C", WMenu.cancel),
    kpress("AnyModifier+P", WMenu.select_prev),
    kpress("AnyModifier+N", WMenu.select_next),
    kpress("Up", WMenu.select_prev),
    kpress("Down", WMenu.select_next),
    kpress("Return", WMenu.finish),
    kpress("KP_Enter", WMenu.finish),
    kpress("Control+M", WMenu.finish),
}

