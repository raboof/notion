--
-- Ion floatws module configuration file
--

-- Bindings for PWM-style floating frame workspaces. These should work
-- on any object on the workspace.

floatws_bindings{
    kpress(DEFAULT_MOD.."Tab",
           function(ws) ws:circulate():raise() end),
    submap(DEFAULT_MOD.."K") { 
        kpress("AnyModifier+Tab",
               function(ws) ws:backcirculate():raise() end),
    },
    kpress(DEFAULT_MOD.."P", function(ws, curr) curr:lower() end),
    kpress(DEFAULT_MOD.."N", function(ws, curr) curr:raise() end),
}


-- Frame bindings. These work in (floating/PWM-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ion-bindings.lua.

floatframe_bindings{
    kpress(DEFAULT_MOD.."R", WFloatFrame.begin_kbresize),
    
    mpress("Button1", WRegion.raise, "tab"),
    mpress("Button1", WRegion.raise, "border"),
    mclick("Button1", WFrame.p_switch_tab, "tab"),
    mdrag("Button1", WFloatFrame.p_move, "tab"),
    mdrag("Button1", WFrame.p_resize, "border"),
    mdblclick("Button1", WFloatFrame.toggle_shade, "tab"),
    
    mclick(DEFAULT_MOD.."Button1", WRegion.raise),
    mdrag(DEFAULT_MOD.."Button1", WFloatFrame.p_move),
    
    mclick("Button2", WFrame.p_switch_tab, "tab"),
    mdrag("Button2", WFrame.p_tabdrag, "tab"),
    
    mclick(DEFAULT_MOD.."Button3", WRegion.lower),
    mdrag(DEFAULT_MOD.."Button3", WFrame.p_resize),

    kpress(DEFAULT_MOD.."M", make_menu_fn("ctxmenu-floatframe")),
    mpress("Button3", make_pmenu_fn("ctxmenu-floatframe"), "tab"),
}


