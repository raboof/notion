--
-- Ion main configuration file
--

--
-- Some basic setup
--

-- Set default modifiers. Alt should usually be mapped to Mod1 on
-- XFree86-based systems. The flying window keys are probably Mod3
-- or Mod4; see the output of 'xmodmap'.
MOD1="Mod1+"
MOD2=""

ioncore.set{
    -- Maximum delay between clicks in milliseconds to be considered a
    -- double click.
    dblclick_delay=250,

    -- For keyboard resize, time (in milliseconds) to wait after latest
    -- key press before automatically leaving resize mode (and doing
    -- the resize in case of non-opaque move).
    kbresize_delay=1500,

    -- Opaque resize?
    opaque_resize=false,

    -- Movement commands warp the pointer to frames instead of just
    -- changing focus. Enabled by default.
    warp=true,
    
    -- Default workspace type.
    default_ws_type="WIonWS",
}


--
-- Load some modules, extensions and other configuration files
--

-- Load some modules.
dopath("mod_query")
dopath("mod_menu")
dopath("mod_ionws")
dopath("mod_floatws")
dopath("mod_panews")
dopath("mod_statusbar")
--dopath("mod_dock")
--dopath("mod_sp")

-- Load some kludges to make apps behave better.
dopath("cfg_kludges")

-- Make some bindings.
dopath("cfg_bindings")

-- Define some menus (mod_menu required)
dopath("cfg_menus")

dopath("cfg_user", true)
