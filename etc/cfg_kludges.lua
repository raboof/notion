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
