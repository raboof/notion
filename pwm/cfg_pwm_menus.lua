--
-- PWM menu definitions
--

-- Main menu
defmenu("mainmenu", {
    submenu("Programs",         "appmenu"),
    menuentry("Lock screen",    "ioncore.exec('xlock')"),
    submenu("Workspaces",       "wsmenu"),
    submenu("Styles",           "stylemenu"),
    submenu("Session",          "sessionmenu"),
})


-- Application menu
defmenu("appmenu", {
    menuentry("XTerm",          "ioncore.exec_on(_, 'xterm')"),
    menuentry("Mozilla Firefox","ioncore.exec_on(_, 'firefox')"),
    menuentry("Xdvi",           "ioncore.exec_on(_, 'xdvi')"),
    menuentry("GV",             "ioncore.exec_on(_, 'gv')"),
})


-- Session control menu
defmenu("sessionmenu", {
    menuentry("Save",           "ioncore.snapshot()"),
    menuentry("Restart",        "ioncore.restart()"),
    menuentry("Restart Ion",    "ioncore.restart_other('ion')"),
    menuentry("Restart TWM",    "ioncore.restart_other('twm')"),
    menuentry("Exit",           "ioncore.shutdown()"),
})


-- Workspaces
defmenu("wsmenu", {
    menuentry("New",            "ioncore.create_ws(_)"),
    menuentry("Close",          "WRegion.rqclose(_sub)",
                                "_sub:WGenWS"),
    submenu("List",             "workspacelist"),
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

