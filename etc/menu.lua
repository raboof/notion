--
-- Menu module configuration. 
-- 
-- Only bindings that are effect in menus are configured here. 
-- See ion-menus.lua for menu definitions and ion-bindings.lua
-- for bindings to display menus.
--


defbindings("WMenu", {
    kpress("Escape", "cancel"),
    kpress("Control+G", "cancel"),
    kpress("Control+C", "cancel"),
    kpress("AnyModifier+P", "select_prev"),
    kpress("AnyModifier+N", "select_next"),
    kpress("Up", "select_prev"),
    kpress("Down", "select_next"),
    kpress("Return", "finish"),
    kpress("KP_Enter", "finish"),
    kpress("Control+M", "finish"),
})

