--
-- Ion floatws module configuration file
--

floatws_bindings{
    kpress(DEFAULT_MOD .. "Tab",
           function(ws)
               f=floatws_circulate(ws)
               floatframe_raise(f)
           end
          ),
    submap "Mod1+K" {
        kpress("AnyModifier+Tab",
               function(ws)
                   f=floatws_backcirculate(ws)
                   floatframe_raise(f)
               end
              ),
    }
}

include("common-frame-bindings.lua")
floatframe_bindings(common_frame_bindings())
floatframe_bindings(frame_query_bindings())

floatframe_bindings{
    mpress("Button1", floatframe_raise, "tab"),
    mpress("Button1", floatframe_raise, "border"),
    mclick("Button1", genframe_p_switch_tab, "tab"),
    mdrag("Button1", floatframe_p_move, "tab"),
    mdrag("Button1", genframe_p_resize, "border"),
    
    mclick(DEFAULT_MOD .. "Button1", floatframe_raise),
    mdrag(DEFAULT_MOD .. "Button1", floatframe_p_move),
    
    mclick("Button2", genframe_p_switch_tab, "tab"),
    mdrag("Button2", genframe_p_tabdrag, "tab"),
    
    mclick(DEFAULT_MOD .. "Button3", floatframe_lower),
    mdrag(DEFAULT_MOD .. "Button3", genframe_p_resize),
}
