-- 
-- PWM bindings configuration file. Global bindings and bindings common 
-- to screens and all types of frames only. See modules' configuration 
-- files for other bindings.
--

-- Load Ion bindings
dopath("cfg_bindings")

-- Unbind anything using mod_query and rebinding to mod_menu where
-- applicable.

defbindings("WScreen", {
    kpress(ALTMETA.."F12", "mod_menu.menu(_, _sub, 'mainmenu', {big=true})"),
})

defbindings("WMPlex", {
    kpress(ALTMETA.."F1", nil),
    kpress(META..   "F1", "ioncore.exec_on(_, ':man pwm3')"),
    kpress(ALTMETA.."F3", nil),
    kpress(META..   "F3", nil),
    kpress(ALTMETA.."F4", nil),
    kpress(ALTMETA.."F5", nil),
    kpress(ALTMETA.."F6", nil),
    kpress(ALTMETA.."F9", nil),
    kpress(META.."G", nil),
})

defbindings("WFrame", {
    kpress(META.."A", nil),
    kpress(META.."M", "mod_menu.menu(_, _sub, 'ctxmenu')"),
})

