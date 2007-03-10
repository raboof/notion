--
-- Options to get some programs work more nicely (or at all)
--


defwinprop{
    class = "AcroRead",
    instance = "documentShell",
    acrobatic = true
}

-- Galeon's find dialog does not always have its transient_for hint 
-- set when the window is being mapped.
defwinprop{
    class = "galeon_browser",
    instance = "dialog_find",
    transient_mode = "current",
}

defwinprop{
    class = "Xpdf",
    instance = "openDialog_popup",
    ignore_cfgrq = true,
}


-- You might want to enable these if you really must use XMMS. 
--[[
defwinprop{
    class = "xmms",
    instance = "XMMS_Playlist",
    transient_mode = "off"
}

defwinprop{
    class = "xmms",
    instance = "XMMS_Player",
    transient_mode = "off"
}
--]]



-- Define some additional title shortening rules to use when the full
-- title doesn't fit in the available space. The first-defined matching 
-- rule that succeeds in making the title short enough is used.
ioncore.defshortening("(.*) - Mozilla(<[0-9]+>)", "$1$2$|$1$<...$2")
ioncore.defshortening("(.*) - Mozilla", "$1$|$1$<...")
ioncore.defshortening("XMMS - (.*)", "$1$|...$>$1")
ioncore.defshortening("[^:]+: (.*)(<[0-9]+>)", "$1$2$|$1$<...$2")
ioncore.defshortening("[^:]+: (.*)", "$1$|$1$<...")
ioncore.defshortening("(.*)(<[0-9]+>)", "$1$2$|$1$<...$2")
ioncore.defshortening("(.*)", "$1$|$1$<...")
