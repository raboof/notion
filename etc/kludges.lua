--
-- Options to get some programs work more nicely (or at all)
--

winprop{
    class = "AcroRead",
    instance = "documentShell",
    acrobatic = true
}

-- Galeon's find dialog does not always have its transient_for hint 
-- set when the window is being mapped.
winprop{
    class = "galeon_browser",
    instance = "dialog_find",
    transient_mode = "current",
}

-- You might want to enable these if you really must use XMMS. 
--[[
winprop{
    class = "xmms",
    instance = "XMMS_Playlist",
    transient_mode = "off"
}

winprop{
    class = "xmms",
    instance = "XMMS_Player",
    transient_mode = "off"
}
--]]
