--
-- PWM menu definitions
--

-- Load the Ion menu setup
dopath("cfg_menus")

-- And make a new main menu with additional workspace menu.

defmenu("mainmenu", {
    submenu("Programs",      "appmenu"),
    menuentry("Lock screen", "ioncore.exec_on(_, 'xlock')"),
    menuentry("Help",        "ioncore.exec_on(_, ':man pwm3')"),
    submenu("Workspaces",    "wsmenu"),
    menuentry("New",         "ioncore.create_ws(_)"),
    submenu("Styles",        "stylemenu"),
    submenu("Session",       "sessionmenu"),
})


-- Workspaces
defmenu("wsmenu", {
    menuentry("New",         "ioncore.create_ws(_)"),
    menuentry("Close",       "WRegion.rqclose(_sub)",
                             "_sub:WGenWS"),
    submenu("List",          "workspacelist"),
})

