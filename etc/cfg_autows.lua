--
-- Ion autows module configuration file
--

-- Bindings for unused area. 

defbindings("WUnusedWin", {
    bdoc("Begin move/resize mode."),
    kpress(MOD1.."R", "WUnusedWin.begin_kbresize(_)"),
    
    bdoc("Resize the area."),
    mdrag(MOD1.."Button3", "WUnusedWin.p_resize(_)"),
    mdrag(MOD1.."Button1", "WUnusedWin.p_move(_)"),
})
