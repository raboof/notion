--
-- PWM menu definitions
--

-- The TR("string") calls below are used to translate the string to
-- the your chosen language, if a translation of Ion exists for it.

-- Load the Ion menu setup
dopath("cfg_menus")

-- And make a new main menu with additional workspace menu.

defmenu("mainmenu", {
    submenu(TR("Programs"),     "appmenu"),
    menuentry(TR("Lock screen"),"ioncore.exec_on(_, 'xlock')"),
    menuentry(TR("Help"),       "mod_query.query_man(_)"),
    menuentry(TR("About Ion"),  "mod_query.show_about_ion(_)"),
    submenu(TR("Workspaces"),   "wsmenu"),
    submenu(TR("Styles"),       "stylemenu"),
    submenu(TR("Session"),      "sessionmenu"),
})


-- Workspaces
defmenu("wsmenu", {
    menuentry(TR("New"),        "ioncore.create_ws(_)"),
    menuentry(TR("Close"),      "WRegion.rqclose(_sub)",
                                "_sub:WGenWS"),
    submenu(TR("List"),         "workspacelist"),
})

