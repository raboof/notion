--
-- Menu module configuration. 
-- 
-- Only bindings that are effect in menus are configured here. 
-- See ion-menus.lua for menu definitions and ion-bindings.lua
-- for bindings to display menus.
--


menu_bindings{
    kpress("Escape", WMenu.cancel),
    kpress("Control+G", WMenu.cancel),
    kpress("Control+C", WMenu.cancel),
    kpress("AnyModifier+P", WMenu.select_prev),
    kpress("AnyModifier+N", WMenu.select_next),
    kpress("Up", WMenu.select_prev),
    kpress("Down", WMenu.select_next),
    kpress("Return", WMenu.finish),
    kpress("KP_Enter", WMenu.finish),
    kpress("Control+M", WMenu.finish),
}

