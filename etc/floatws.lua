--
-- Ion floatws module configuration file
--

-- Bindings for PWM-style floating frame workspaces. These should work
-- on any object on the workspace.

defbindings("WFloatWS", {
    kpress(MOD1.."Tab",         "WFloatWS.raise(_, WFloatWS.circulate(_))"),
    submap(MOD1.."K", { 
        kpress("AnyModifier+Tab", "WFloatWS.raise(_, WFloatWS.backcirculate(_))"),
    }),
    kpress(MOD1.."P",           "WFloatWS.lower(_, _sub)", "_sub:non-nil"),
    kpress(MOD1.."N",           "WFloatWS.raise(_, _sub)", "_sub:non-nil"),
})


-- Frame bindings. These work in (floating/PWM-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ion-bindings.lua.

defbindings("WFloatFrame", {
    mdblclick("Button1@tab",    "WFloatFrame.toggle_shade(_)"),
    mpress("Button1@tab",       "WFloatWS.raise(WRegion.manager(_), _)"),
    mpress("Button1@border",    "WFloatWS.raise(WRegion.manager(_), _)"),
    mclick(MOD1.."Button1",     "WFloatWS.raise(WRegion.manager(_), _)"),
    mclick(MOD1.."Button3",     "WFloatWS.lower(WRegion.manager(_), _)"),
    mdrag("Button1@tab",        "WFrame.p_move(_)"),
     -- in ion-bindings.lua now:
    --mdrag(MOD1.."Button1",      "WFrame.p_move(_)"),
    
    kpress(MOD1.."M",           "menulib.menu(_, _sub, 'ctxmenu-floatframe')"),
    mpress("Button3@tab",       "menulib.pmenu(_, _sub, 'ctxmenu-floatframe')"),
})

