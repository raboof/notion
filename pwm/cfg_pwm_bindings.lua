-- 
-- PWM bindings configuration file. Global bindings and bindings common 
-- to screens and all types of frames only. See modules' configuration 
-- files for other bindings.
--

-- Load Ion bindings
dopath("cfg_bindings")

-- Unbind anything using mod_query.

defbindings("WMPlex", {
    kpress(MOD2.."F1", nil),
    kpress(MOD2.."F3", nil),
    kpress(MOD1.."F3", nil),
    kpress(MOD2.."F4", nil),
    kpress(MOD2.."F5", nil),
    kpress(MOD2.."F6", nil),
    kpress(MOD2.."F9", nil),
    kpress(MOD1.."G", nil),
})


defbindings("WFrame", {
    kpress(MOD1.."A", nil),
})

