--
-- Ion menu definitions
--

-- The TR("string") calls below are used to translate the string to
-- the your chosen language, if a translation of Ion exists for it.

-- Main menu
defmenu("mainmenu", {
    submenu(TR("Programs"),     "appmenu"),
    menuentry(TR("Lock screen"),"ioncore.exec_on(_, 'xlock')"),
    menuentry(TR("Help"),       "mod_query.query_man(_)"),
    menuentry(TR("About Ion"),  "mod_query.show_about_ion(_)"),
    submenu(TR("Styles"),       "stylemenu"),
    submenu(TR("Session"),      "sessionmenu"),
})


-- Application menu
defmenu("appmenu", {
    menuentry("XTerm",          "ioncore.exec_on(_, 'xterm')"),
    menuentry("Mozilla Firefox","ioncore.exec_on(_, 'firefox')"),
    menuentry(TR("Run..."),     "mod_query.query_exec(_)"),
})


-- Session control menu
defmenu("sessionmenu", {
    menuentry(TR("Save"),       "ioncore.snapshot()"),
    menuentry(TR("Restart"),    "ioncore.restart()"),
    menuentry(TR("Restart PWM"),"ioncore.restart_other('pwm')"),
    menuentry(TR("Restart TWM"),"ioncore.restart_other('twm')"),
    menuentry(TR("Exit"),       "ioncore.shutdown()"),
})


-- Context menu (frame/client window actions)
defmenu("ctxmenu", {
    menuentry(TR("Close"),      "WRegion.rqclose_propagate(_, _sub)"),
    menuentry(TR("Kill"),       "WClientWin.kill(_sub)",
                                "_sub:WClientWin"),
    menuentry(TR("(Un)tag"),    "WRegion.toggle_tag(_sub)",
                                "_sub:non-nil"),
    menuentry(TR("Attach tagged"), "WFrame.attach_tagged(_)"),
    menuentry(TR("Clear tags"), "ioncore.clear_tags()"),
    menuentry(TR("Window info"),"mod_query.show_clientwin(_, _sub)",
                                "_sub:WClientWin"),
})


-- Context menu for floating frames -- add sticky toggle.
defmenu("ctxmenu-floatframe", {
    menuentry(TR("Close"),      "WRegion.rqclose_propagate(_, _sub)"),
    menuentry(TR("Kill"),       "WClientWin.kill(_sub)",
                                "_sub:WClientWin"),
    menuentry(TR("(Un)tag"),    "WRegion.toggle_tag(_sub)",
                                "_sub:non-nil"),
    menuentry(TR("Attach tagged"), "WFrame.attach_tagged(_)"),
    menuentry(TR("Clear tags"), "ioncore.clear_tags()"),
    menuentry(TR("(Un)stick"),  "WFloatFrame.toggle_sticky(_)"),
})

