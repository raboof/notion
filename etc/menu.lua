--
-- Menu module configuration. 
-- 
-- Only bindings that are effect in menus are configured here. 
-- See ion-menus.lua for menu definitions and ion-bindings.lua
-- for bindings to display menus.
--


defbindings("WMenu", {
    kpress("Escape", 		"WMenu.cancel(_)"),
    kpress("Control+G",		"WMenu.cancel(_)"),
    kpress("Control+C", 	"WMenu.cancel(_)"),
    kpress("Return",		"WMenu.finish(_)"),
    kpress("KP_Enter",		"WMenu.finish(_)"),
    kpress("Control+M", 	"WMenu.finish(_)"),
    kpress("AnyModifier+P", 	"WMenu.select_prev(_)"),
    kpress("AnyModifier+N", 	"WMenu.select_next(_)"),
    kpress("Up", 		"WMenu.select_prev(_)"),
    kpress("Down", 		"WMenu.select_next(_)"),
})

