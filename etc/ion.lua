--
-- Ion main configuration file
--

-- Set default modifier. Alt should usually be mapped to Mod1 on
-- XFree86-based systems. The flying window keys are probably Mod3
-- or Mod4; see the output of 'xmodmap'.
MOD1="Mod1+"
MOD2=""

-- Maximum delay between clicks in milliseconds to be considered a
-- double click.
--ioncore.set_dblclick_delay(250)

-- For keyboard resize, time (in milliseconds) to wait after latest
-- key press before automatically leaving resize mode (and doing
-- the resize in case of non-opaque move).
--ioncore.set_resize_delay(1500)

-- Opaque resize?
ioncore.set_opaque_resize(false)

-- Movement commands warp the pointer to frames instead of just
-- changing focus. Enabled by default.
ioncore.set_warp(true)

-- Kludges to make apps behave better.
include("kludges")

-- Some usefull routines (needed by ion-bindings and ion-menus)
include("menulib")
include("querylib")

-- Make some bindings.
include("ion-bindings")

-- Define some menus (menu module required to actually use them)
include("ion-menus")

-- How to shorten window titles when the full title doesn't fit in
-- the available space? The first-defined matching rule that succeeds 
-- in making the title short enough is used.
ioncore.add_shortenrule("(.*) - Mozilla(<[0-9]+>)", "$1$2$|$1$<...$2")
ioncore.add_shortenrule("(.*) - Mozilla", "$1$|$1$<...")
ioncore.add_shortenrule("XMMS - (.*)", "$1$|...$>$1")
ioncore.add_shortenrule("[^:]+: (.*)(<[0-9]+>)", "$1$2$|$1$<...$2")
ioncore.add_shortenrule("[^:]+: (.*)", "$1$|$1$<...")
ioncore.add_shortenrule("(.*)(<[0-9]+>)", "$1$2$|$1$<...$2")
ioncore.add_shortenrule("(.*)", "$1$|$1$<...")

-- Modules.
ioncore.load_module("querymod")
ioncore.load_module("menumod")
ioncore.load_module("ionwsmod")
ioncore.load_module("floatwsmod")
--ioncore.load_module("dockmod")
