-- Authors: tyranix
-- License: Public domain
-- Last Changed: Unknown
-- 
-- rotate_statusbar.lua
--[[
This replacement for cfg_statusbar.lua will rotate between
statusbars at a user defined interval.  This is handy for
status information you are not interested in all the time such
as what song you are playing or wireless strength and so on.

Usage:

1) Back-up the existing ~/.ion3/cfg_statusbar.lua (if present).
   Rename this file called rotate_statusbar.lua to ~/.ion3/cfg_statusbar.lua.

2) Make changes to rotate_statusbar.settings.allbars (see below "USER DEFINED").
   Each table represents a new status bar that will be shown.

3) For further changes, look into save_statusbar.lua, which will
   be created by this script). Also notice init_settings and configurations.

4) Now restart ion3 (hit 'F12' then type 'session/restart')


This is heavily based on ctrl_statusbar.lua by Sadrul Habib Chowdhury.

-tyranix (all public domain)


TODO:

  All meters (whether visible or not) run in the background.  It would be
  nice to only run the visible meters to avoid wasting CPU.
    - Can I access that functionality from here?

--]]

if not mod_statusbar then return end

rotate_statusbar = { counter=0 }

-- Initializing settings for statusbar
-- This is similar to what is done in mod_statusbar.create in cfg_statusbar.lua
if not rotate_statusbar.init_settings then
    rotate_statusbar.init_settings = {
        screen=0,       -- First screen
        pos='bl',       -- Bottom left corner
        fullsize=false,
        systray=true,   -- In the systray
        
        -- Date format goes here instead of in the configurations below because
        -- it is loaded by ion3?
        -- date_format='%a %Y-%b-%d %H:%M',
    }
end

-- Settings for individual modules to override the defaults in their
-- respective files.
--
-- To find the defaults, for instance, foo = {} would be in statusd_foo.lua.
--
-- Similar to the settings in mod_statusbar.launch_statusd in cfg_statusbar.lua
if not rotate_statusbar.configurations then
    rotate_statusbar.configurations = {
        -- Load meter
        --[[
        load = {
            update_interval=10*1000,
            important_threshold=1.5,
            critical_threshold=4.0
        },
        --]]
    
        -- Mail meter
        --[[
        mail = {
            update_interval=60*1000,
            mbox=os.getenv("MAIL")
        },
        --]]
    }
end

-- USER DEFINED User should change these
if not rotate_statusbar.settings then
    rotate_statusbar.settings = {
        -- Change statusbars every 60 seconds.
        update_interval = 60*1000,

        -- Add a table for each status bar you want to have rotated.
        -- Template. Tokens %string are replaced with the value of the 
        -- corresponding meter.
        --
        -- Space preceded by % adds stretchable space for alignment of variable
        -- meter value widths. > before meter name aligns right using this 
        -- stretchable space , < left, and | centers.
        -- Meter values may be zero-padded to a width preceding the meter name.
        -- These alignment and padding specifiers and the meter name may be
        -- enclosed in braces {}.
        --
        -- %filler causes things on the marker's sides to be aligned left and
        -- right, respectively, and %systray is a placeholder for system tray
        -- windows and icons.
        all_statusbars = {
	   -- Make sure the defaults work for people.
	   "[ %date ]",
	   "[ %date || %load ]",
	   "[ %date || %load || %date ]",

	   -- Examples of other usage
           -- "[ %date || %load || %mocp_title (%mocp_currenttime %mocp_timeleft) ]",
	   -- "[ %date || %load || %uptime ]",
	   -- "[ %date || %load || %netmon ]",
        },
    }
end

-- Construct the template for the statusbar
function rotate_statusbar.get_template()
    -- Rotate through the pre-defined status bars
    if rotate_statusbar.counter >= table.getn(rotate_statusbar.settings.all_statusbars) then
        rotate_statusbar.counter = 0
    end
    rotate_statusbar.counter = rotate_statusbar.counter + 1

    return rotate_statusbar.settings.all_statusbars[rotate_statusbar.counter]
end

-- Create and initialize the statusbar
function rotate_statusbar.init()
    rotate_statusbar.init_settings.template = rotate_statusbar.settings.all_statusbars[1]

    -- This is a very ugly hack to make sure all the statusd_ scripts get loaded
    -- Build a template with all the variables so the C program is called with
    -- the right modules to load.
    for _, t in pairs(rotate_statusbar.settings.all_statusbars) do
        rotate_statusbar.init_settings.template = rotate_statusbar.init_settings.template .. " " .. t
    end
    rotate_statusbar.sb = mod_statusbar.create(rotate_statusbar.init_settings)
    mod_statusbar.launch_statusd(rotate_statusbar.configurations)
 end

-- Refresh the statusbar continually
function update_rotate_statusbar()
    local st = rotate_statusbar.get_template()
    local tbl = mod_statusbar.template_to_table(st)

    rotate_statusbar.sb:set_template_table(tbl)

    rotate_statusbar_timer:set(rotate_statusbar.settings.update_interval,
                               update_rotate_statusbar)
end

-- Initialize the statusd with the initial configuration
rotate_statusbar.init()

-- Create a timer to continually change the status bar
-- rotate_statusbar_timer = statusd.create_timer()
rotate_statusbar_timer = ioncore.create_timer()
update_rotate_statusbar()
