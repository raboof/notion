--
-- PWM main configuration file
--
-- This file only includes some settings that are rather frequently altered,
-- and the differences between PWM and Ion. The rest of the settings are in 
-- cfg_ioncore.lua and individual modules' configuration files 
-- (cfg_modulename.lua).
--

-- Set default modifiers. Alt should usually be mapped to Mod1 on
-- XFree86-based systems. The flying window keys are probably Mod3
-- or Mod4; see the output of 'xmodmap'.
--META="Mod1+"
--ALTMETA=""

-- Some basic settings
ioncore.set{
    -- Maximum delay between clicks in milliseconds to be considered a
    -- double click.
    --dblclick_delay=250,

    -- For keyboard resize, time (in milliseconds) to wait after latest
    -- key press before automatically leaving resize mode (and doing
    -- the resize in case of non-opaque move).
    --kbresize_delay=1500,

    -- Opaque resize?
    --opaque_resize=false,

    -- Movement commands warp the pointer to frames instead of just
    -- changing focus. Enabled by default.
    --warp=true,
}

-- cfg_ioncore contains configuration of the Ion 'core'
dopath("cfg_ioncore")

-- Load some modules. 
--dopath("cfg_modules")
--dopath("mod_query")
dopath("mod_menu")
--dopath("mod_tiling")
--dopath("mod_statusbar")
dopath("mod_dock")
--dopath("mod_sp")


--
-- PWM customisations to bindings and menus
--


-- Unbind anything using mod_query and rebind to mod_menu where
-- applicable.

defbindings("WScreen", {
    bdoc("Display the main menu."),
    kpress(ALTMETA.."F12", "mod_menu.menu(_, _sub, 'mainmenu', {big=true})"),
})

defbindings("WMPlex.toplevel", {
    kpress(ALTMETA.."F1", nil),
    kpress(ALTMETA.."F3", nil),
    kpress(META..   "F3", nil),
    kpress(ALTMETA.."F4", nil),
    kpress(ALTMETA.."F5", nil),
    kpress(ALTMETA.."F6", nil),
    kpress(ALTMETA.."F9", nil),
    kpress(META.."G", nil),
    
    bdoc("Show the PWM manual page."),
    kpress(META..   "F1", "ioncore.exec_on(_, ':man pwm3')"),
    
    bdoc("Display context menu."),
    kpress(META.."M", "mod_menu.menu(_, _sub, 'ctxmenu')"),
})

defbindings("WFrame.toplevel", {
    kpress(META.."A", nil),
})

-- Make a new main menu with additional workspace menu.

defmenu("mainmenu", {
    submenu("Programs",      "appmenu"),
    menuentry("Lock screen", "ioncore.exec_on(_, 'xlock')"),
    menuentry("Help",        "ioncore.exec_on(_, ':man pwm3')"),
    submenu("Workspaces",    "wsmenu"),
    submenu("Styles",        "stylemenu"),
    submenu("Session",       "sessionmenu"),
})

-- Application menu
defmenu("appmenu", {
    menuentry("XTerm",          "ioncore.exec_on(_, 'xterm')"),
    menuentry("W3M",            "ioncore.exec_on(_, ':w3m -v')"),
    menuentry("Rxvt",           "ioncore.exec_on(_, 'rxvt')"),
    menuentry("Opera",          "ioncore.exec_on(_, 'opera')"),
    menuentry("Links",          "ioncore.exec_on(_, ':links')"),
    menuentry("Konqueror",      "ioncore.exec_on(_, 'konqueror')"),
    menuentry("Dillo",          "ioncore.exec_on(_, 'dillo')"),
    menuentry("Run...",         "mod_query.query_exec(_)"),
})

-- Workspace menu
defmenu("wsmenu", {
    menuentry("New",         "ioncore.create_ws(_)"),
    menuentry("Close",       "WRegion.rqclose(_sub)",
                             "_sub:WGroupWS"),
    submenu("List",          "workspacelist"),
})

