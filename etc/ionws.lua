--
-- Ion ionws module configuration file
--

-- Bindings for the tiled workspaces (ionws). These should work on any 
-- object on the workspace.

defbindings("WIonWS", {
    kpress(DEFAULT_MOD.."N", "goto_below"),
    kpress(DEFAULT_MOD.."P", "goto_above"),
    kpress(DEFAULT_MOD.."Tab", "goto_right"),
    submap(DEFAULT_MOD.."K", {
        kpress("AnyModifier+Tab", "goto_left"),
    }),
})


-- Frame bindings. These work in (Ion/tiled-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ion-bindings.lua.

defbindings("WIonFrame", {
    kpress(DEFAULT_MOD.."S", "split", "bottom"),
    submap(DEFAULT_MOD.."K", {
        --kpress("AnyModifier+T", "@sub_cwin", "toggle_transients_pos"),
        kpress("AnyModifier+X", "relocate_and_close"),
        kpress("AnyModifier+S", "split", "right"),
    }),
    
    mclick("Button1@tab", "p_switch_tab"),
    mdblclick("Button1@tab", "toggle_shade"),
    mdrag("Button1@tab", "p_tabdrag"),
    mdrag("Button1@border", "p_resize"),
    
    mclick("Button2@tab", "p_switch_tab"),
    mdrag("Button2@tab", "p_tabdrag"),
    
    mdrag(DEFAULT_MOD.."Button3", "p_resize"),
})
