--
-- Ion floatws module configuration file
--

-- Bindings for PWM-style floating frame workspaces. These should work
-- on any object on the workspace.

defbindings("WFloatWS", {
    kpress(DEFAULT_MOD.."Tab", "circulate_and_raise"),
    submap(DEFAULT_MOD.."K", { 
        kpress("AnyModifier+Tab", "backcirculate_and_raise"),
    }),
    kpress(DEFAULT_MOD.."P", "@sub", "lower"),
    kpress(DEFAULT_MOD.."N", "@sub", "raise"),
})


-- Frame bindings. These work in (floating/PWM-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ion-bindings.lua.

defbindings("WFloatFrame", {
    mpress("Button1@tab", "raise"),
    mpress("Button1@border", "raise"),
    mclick("Button1@tab", "p_switch_tab"),
    mdrag("Button1@tab", "p_move"),
    mdrag("Button1@border", "p_resize"),
    mdblclick("Button1@tab", "toggle_shade"),
    
    mclick(DEFAULT_MOD.."Button1", "raise"),
    mdrag(DEFAULT_MOD.."Button1", "p_move"),
    
    mclick("Button2@tab", "p_switch_tab"),
    mdrag("Button2@tab", "p_tabdrag"),
    
    mclick(DEFAULT_MOD.."Button3", "lower"),
    mdrag(DEFAULT_MOD.."Button3", "p_resize"),

    kpress(DEFAULT_MOD.."M", "menu", "ctxmenu-floatframe"),
    mpress("Button3@tab", "pmenu", "ctxmenu-floatframe"),
})

