--
-- Menu module configuration. 
-- 
-- Only bindings that are effect in menus are configured here. 
-- See ion-menus.lua for menu definitions and ion-bindings.lua
-- for bindings to display menus.
--


defbindings("WMenu", {
    bdoc("Close the menu."),
    kpress("Escape", "WMenu.cancel(_)"),
    kpress("Control+G", "WMenu.cancel(_)"),
    kpress("Control+C", "WMenu.cancel(_)"),
    kpress("Left", "WMenu.cancel(_)"),
    
    bdoc("Activate current menu entry."),
    kpress("Return",  "WMenu.finish(_)"),
    kpress("KP_Enter", "WMenu.finish(_)"),
    kpress("Control+M", "WMenu.finish(_)"),
    kpress("Right", "WMenu.finish(_)"),
    
    bdoc("Select next/previous menu entry."),
    kpress("Control+N", "WMenu.select_next(_)"),
    kpress("Control+P", "WMenu.select_prev(_)"),
    kpress("Up", "WMenu.select_prev(_)"),
    kpress("Down", "WMenu.select_next(_)"),
    
    bdoc("Clear the menu's typeahead find buffer."),
    kpress("BackSpace", "WMenu.typeahead_clear(_)"),
})

