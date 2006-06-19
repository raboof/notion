--
-- Ion main configuration file
--
-- This file only includes some settings that are rather frequently altered.
-- The rest of the settings are in cfg_ioncore.lua and individual modules'
-- configuration files (cfg_modulename.lua).
--

-- Set default modifiers. Alt should usually be mapped to Mod1 on
-- XFree86-based systems. The flying window keys are probably Mod3
-- or Mod4; see the output of 'xmodmap'.
--META="Mod1+"
--ALTMETA=""

-- Terminal emulator
--XTERM="xterm"

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
    
    -- Default workspace type.
    --default_ws_type="WIonWS",
}

-- cfg_ioncore contains configuration of the Ion 'core'
dopath("cfg_ioncore")

-- Load some kludges to make apps behave better.
dopath("cfg_kludges")

-- Load some modules. Disable the loading of cfg_modules by commenting out 
-- the corresponding line with -- if you don't want the whole default set 
-- (everything except mod_dock). Then uncomment the lines for the modules
-- you want. 
dopath("cfg_modules")
--dopath("mod_query")
--dopath("mod_menu")
--dopath("mod_ionws")
--dopath("mod_floatws")
--dopath("mod_panews")
--dopath("mod_statusbar")
--dopath("mod_dock")
--dopath("mod_sp")

-- Deprecated.
dopath("cfg_user", true)
