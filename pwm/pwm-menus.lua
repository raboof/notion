--
-- PWM menu definitions
--


-- Load a library with some convenience functions.
include("menulib")
-- Load query support code
include("querylib")


-- Main menu
defmenu("mainmenu", {
    submenu("Programs", "appmenu"),
    menuentry("Lock screen", make_exec_fn("xlock")),
    submenu("Workspaces", "wsmenu"),
    submenu("Styles", "stylemenu"),
    submenu("Exit", "exitmenu"),
})


-- Application menu
defmenu("appmenu", {
    menuentry("XTerm", make_exec_fn("xterm")),
    menuentry("Mozilla Firebird", make_exec_fn("MozillaFirebird")),
    menuentry("Xdvi", make_exec_fn("xdvi")),
    menuentry("GV", make_exec_fn("gv")),
})


-- Menu with restart/exit alternatives
defmenu("exitmenu", {
    --menuentry("Restart", querylib.query_restart),
    menuentry("Restart", restart_wm),
    menuentry("Restart Ion", function() restart_other_wm("ion") end),
    menuentry("Restart TWM", function() restart_other_wm("twm") end),
    --menuentry("Exit", querylib.query_exit),
    menuentry("Exit", exit_wm),
})


-- Workspaces
defmenu("wsmenu", {
    menuentry("New", function(m) 
                         m:screen_of():attach_new({
                             type=(default_ws_type or "WFloatWS"), 
                             switchto=true,}) 
                     end),
    menuentry("Close", function(m) m:screen_of():current():close() end),
    submenu("List", "workspacelist"),
})


-- Context menu (frame/client window actions)
defmenu("ctxmenu", {
    menuentry("Close", WMPlex.close_sub_or_self),
    menuentry("Kill", make_mplex_clientwin_fn(WClientWin.kill)),
    menuentry("(Un)tag", make_mplex_sub_fn(WRegion.toggle_tag)),
    menuentry("Attach tagged", WGenFrame.attach_tagged),
    menuentry("Clear tags", clear_tags),
})


-- Context menu for floating frames -- add sticky toggle.
defmenu("ctxmenu-floatframe", {
    menuentry("Close", WMPlex.close_sub_or_self),
    menuentry("Kill", make_mplex_clientwin_fn(WClientWin.kill)),
    menuentry("(Un)tag", make_mplex_sub_fn(WRegion.toggle_tag)),
    menuentry("Attach tagged", WGenFrame.attach_tagged),
    menuentry("Clear tags", clear_tags),
    menuentry("(Un)stick", function(f) f:toggle_sticky() end),
})

