--
-- PWM menu definitions
--


-- Load a library with some convenience functions.
include("menulib")


-- Main menu
defmenu("mainmenu", {
    submenu("Programs", "appmenu"),
    menuentry("Lock screen", "exec", "xlock"),
    submenu("Workspaces", "wsmenu"),
    submenu("Styles", "stylemenu"),
    submenu("Exit", "exitmenu"),
})


-- Application menu
defmenu("appmenu", {
    menuentry("XTerm", "exec", "xterm"),
    menuentry("Mozilla Firebird", "exec", "MozillaFirebird"),
    menuentry("Xdvi", "exec", "xdvi"),
    menuentry("GV", "exec", "gv"),
})


-- Menu with restart/exit alternatives
defmenu("exitmenu", {
    menuentry("Restart", "restart_wm"),
    menuentry("Restart Ion", "restart_other_wm", "ion"),
    menuentry("Restart TWM", "restart_other_wm", "twm"),
    menuentry("Exit", "exit_wm"),
})


-- Workspaces
defmenu("wsmenu", {
    menuentry("New", "@screen_of", "create_new_ws"),
    menuentry("Close", "@screen_of", "close_current_ws"),
    submenu("List", "workspacelist"),
})


-- Context menu (frame/client window actions)
defmenu("ctxmenu", {
    menuentry("Close", "close_sub_or_self"),
    menuentry("Kill", "@sub_cwin", "kill"),
    menuentry("(Un)tag", "@sub", "toggle_tag"),
    menuentry("Attach tagged", "attach_tagged"),
    menuentry("Clear tags", "clear_tags"),
})


-- Context menu for floating frames -- add sticky toggle.
defmenu("ctxmenu-floatframe", {
    menuentry("Close", "close_sub_or_self"),
    menuentry("Kill", "@sub_cwin", "kill"),
    menuentry("(Un)tag", "@sub", "toggle_tag"),
    menuentry("Attach tagged", "attach_tagged"),
    menuentry("Clear tags", "clear_tags"),
    menuentry("(Un)stick", "toggle_sticky"),
})

