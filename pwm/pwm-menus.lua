--
-- PWM menu definitions
--

-- Main menu
defmenu("mainmenu", {
    submenu("Programs",         "appmenu"),
    menuentry("Lock screen",    "ioncore.exec('xlock')"),
    menuentry("Help",           "querylib.query_man(_)"),
    menuentry("About Ion",      "querylib.show_about_ion(_)"),
    submenu("Workspaces",       "workspacelist"),
    submenu("Styles",           "stylemenu"),
    submenu("Exit",             "exitmenu"),
})


-- Application menu
defmenu("appmenu", {
    menuentry("XTerm",          "ioncore.exec_on(_, 'xterm')"),
    menuentry("Mozilla Firefox","ioncore.exec_on(_, 'firefox')"),
    menuentry("Xdvi",           "ioncore.exec_on(_, 'xdvi')"),
    menuentry("GV",             "ioncore.exec_on(_, 'gv')"),
})


-- Menu with restart/exit alternatives
defmenu("exitmenu", {
    menuentry("Restart",        "ioncore.restart()"),
    menuentry("Restart Ion",    "ioncore.restart_other('ion')"),
    menuentry("Restart TWM",    "ioncore.restart_other('twm')"),
    menuentry("Exit",           "ioncore.exit()"),
})


-- Context menu (frame/client window actions)
defmenu("ctxmenu", {
    menuentry("Close",          "WRegion.rqclose_propagate(_, _sub)"),
    menuentry("Kill",           "WClientWin.kill(_sub)",
                                "_sub:WClientWin"),
    menuentry("(Un)tag",        "WRegion.toggle_tag(_sub)",
                                "_sub:non-nil"),
    menuentry("Attach tagged",  "WFrame.attach_tagged(_)"),
    menuentry("Clear tags",     "ioncore.clear_tags()"),
})


-- Context menu for floating frames -- add sticky toggle.
defmenu("ctxmenu-floatframe", {
    menuentry("Close",          "WRegion.rqclose_propagate(_, _sub)"),
    menuentry("Kill",           "WClientWin.kill(_sub)",
                                "_sub:WClientWin"),
    menuentry("(Un)tag",        "WRegion.toggle_tag(_sub)",
                                "_sub:non-nil"),
    menuentry("Attach tagged",  "WFrame.attach_tagged(_)"),
    menuentry("Clear tags",     "ioncore.clear_tags()"),
    menuentry("(Un)stick",      "WFloatFrame.toggle_sticky(_)"),
})

