--
-- Ion ionws module configuration file
--

ionws_bindings{
    kpress(DEFAULT_MOD .. "S",
           function(ws, sub) ionws_split(sub, "bottom") end
          ),
    kpress(DEFAULT_MOD .. "N", ionws_goto_below),
    kpress(DEFAULT_MOD .. "P", ionws_goto_above),
    kpress(DEFAULT_MOD .. "Tab", ionws_goto_right),
    submap(DEFAULT_MOD .. "K") {
        kpress("AnyModifier+S",
               function(ws,sub) ionws_split(sub, "right") end
              ),
        kpress("AnyModifier+Tab", ionws_goto_left),
    },
}

include("common-frame-bindings.lua")
ionframe_bindings(common_frame_bindings())
ionframe_bindings(frame_query_bindings())

ionframe_bindings{
    kpress(DEFAULT_MOD .. "H", ionframe_resize_horiz),
    kpress(DEFAULT_MOD .. "V", ionframe_resize_vert),
    
    submap(DEFAULT_MOD .. "K"){
        kpress("X", ionframe_close),
    },
    
    mclick("Button1", genframe_p_switch_tab, "tab"),
    mdrag("Button1", genframe_p_tabdrag, "tab"),
    mdrag("Button1", genframe_p_resize, "border"),
    
    mclick("Button2", genframe_p_switch_tab, "tab"),
    mdrag("Button2", genframe_p_tabdrag, "tab"),
    
    mdrag("Mod1+Button3", genframe_p_resize),
}


ionframe_moveres_bindings{
    kpress("AnyModifier+Escape", ionframe_cancel_resize),
    kpress("AnyModifier+Return", ionframe_end_resize),
    kpress("AnyModifier+V", ionframe_grow),
    kpress("AnyModifier+H", ionframe_grow),
    kpress("AnyModifier+S", ionframe_shrink),
}

