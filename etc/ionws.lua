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
    kpress(DEFAULT_MOD .. "R", ionframe_begin_resize),
    
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


local function ionframe_grow_vert(frame)
    ionframe_do_resize(frame, 0, 1)
end

local function ionframe_grow_horiz(frame)
    ionframe_do_resize(frame, 1, 0)
end

local function ionframe_shrink_vert(frame)
    ionframe_do_resize(frame, 0, -1)
end

local function ionframe_shrink_horiz(frame)
    ionframe_do_resize(frame, -1, 0)
end


ionframe_moveres_bindings{
    kpress("AnyModifier+Escape", ionframe_cancel_resize),
    kpress("AnyModifier+Return", ionframe_end_resize),
    kpress("AnyModifier+Up", ionframe_grow_vert),
    kpress("AnyModifier+Down", ionframe_shrink_vert),
    kpress("AnyModifier+Right", ionframe_grow_horiz),
    kpress("AnyModifier+Left", ionframe_shrink_horiz),
    kpress("AnyModifier+P", ionframe_grow_vert),
    kpress("AnyModifier+N", ionframe_shrink_vert),
    kpress("AnyModifier+F", ionframe_grow_horiz),
    kpress("AnyModifier+B", ionframe_shrink_horiz),
}

