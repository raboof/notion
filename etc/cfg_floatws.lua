--
-- Ion floatws module configuration file
--

-- Bindings for PWM-style floating frame workspaces. These should work
-- on any object on the workspace.

defbindings("WFloatWS", {
    bdoc("Circulate focus and raise the newly focused frame."),
    kpress(MOD1.."Tab", "WFloatWS.raise(_, WFloatWS.circulate(_))"),
    submap(MOD1.."K", { 
        bdoc("Backwards-circulate focus and raise the newly focused frame."),
        kpress("Tab", "WFloatWS.raise(_, WFloatWS.backcirculate(_))"),
    }),
    bdoc("Raise/lower active frame."),
    kpress(MOD1.."P", "WFloatWS.lower(_, _sub)", "_sub:non-nil"),
    kpress(MOD1.."N", "WFloatWS.raise(_, _sub)", "_sub:non-nil"),
})


-- Frame bindings. These work in (floating/PWM-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ion-bindings.lua.

defbindings("WFloatFrame", {
    bdoc("Toggle shade mode"),
    mdblclick("Button1@tab", "WFloatFrame.set_shaded(_, 'toggle')"),
    
    bdoc("Raise the frame."),
    mpress("Button1@tab", "WFloatWS.raise(WRegion.manager(_), _)"),
    mpress("Button1@border", "WFloatWS.raise(WRegion.manager(_), _)"),
    mclick(MOD1.."Button1", "WFloatWS.raise(WRegion.manager(_), _)"),
    
    bdoc("Lower the frame."),
    mclick(MOD1.."Button3", "WFloatWS.lower(WRegion.manager(_), _)"),
    
    bdoc("Move the frame."),
    mdrag("Button1@tab", "WFrame.p_move(_)"),
})


-- WFloatFrame context menu extras

defctxmenu("WFloatFrame", {
    menuentry("(Un)stick",      "WFloatFrame.toggle_sticky(_)"),
})

