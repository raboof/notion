--
-- Ion menu definitions
--


-- Main menu
defmenu("mainmenu", {
    submenu("Programs", "appmenu"),
    menuentry("Lock screen", "exec", "xlock"),
    menuentry("Help", "query_man"),
    menuentry("About Ion", "show_about_ion"),
    submenu("Styles", "stylemenu"),
    submenu("Exit", "exitmenu"),
})


-- Application menu
defmenu("appmenu", {
    menuentry("XTerm", "exec", "xterm"),
    menuentry("Mozilla Firebird", "exec", "MozillaFirebird"),
    -- The query module must also be loaded for this binding to work.
    menuentry("Run...", "query_exec"),
})


-- Menu with restart/exit alternatives
defmenu("exitmenu", {
    --menuentry("Restart", querylib.query_restart),
    menuentry("Restart", "restart_wm"),
    menuentry("Restart PWM", "restart_other_wm", "pwm"),
    menuentry("Restart TWM", "restart_other_wm", "twm"),
    --menuentry("Exit", querylib.query_exit),
    menuentry("Exit", "exit_wm"),
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

