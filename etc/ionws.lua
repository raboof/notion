--
-- Ion ionws module configuration file
--

-- Workspace bindings for the tiled workspaces.
-- These should work on any object on the workspace.

ionws_bindings{
    kpress(DEFAULT_MOD .. "N", ionws_goto_below),
    kpress(DEFAULT_MOD .. "P", ionws_goto_above),
    kpress(DEFAULT_MOD .. "Tab", ionws_goto_right),
    submap(DEFAULT_MOD .. "K") {
        kpress("AnyModifier+Tab", ionws_goto_left),
    },
}

-- Frame bindings. These work in (Ion/tiled-style) frames.

include("common-frame-bindings.lua")
ionframe_bindings(common_frame_bindings())
ionframe_bindings(frame_query_bindings())

ionframe_bindings{
    kpress(DEFAULT_MOD .. "R", ionframe_begin_resize),
    kpress(DEFAULT_MOD .. "S",
           function(frame) ionframe_split(frame, "bottom") end),

    submap(DEFAULT_MOD .. "K"){
        kpress("AnyModifier+X", ionframe_close),
        kpress("AnyModifier+S",
               function(frame) ionframe_split(frame, "right") end),
    },
    
    mclick("Button1", genframe_p_switch_tab, "tab"),
    mdrag("Button1", genframe_p_tabdrag, "tab"),
    mdrag("Button1", genframe_p_resize, "border"),
    
    mclick("Button2", genframe_p_switch_tab, "tab"),
    mdrag("Button2", genframe_p_tabdrag, "tab"),
    
    mdrag(DEFAULT_MOD .. "Button3", genframe_p_resize),
}


-- Frame resize mode bindings

ionframe_moveres_bindings{
    kpress("AnyModifier+Escape", ionframe_cancel_resize),
    kpress("AnyModifier+Return", ionframe_end_resize),
    
    kpress("Left", function(f) ionframe_do_resize(f, 1, 0, 0, 0) end),
    kpress("Right",function(f) ionframe_do_resize(f, 0, 1, 0, 0) end),
    kpress("Up",   function(f) ionframe_do_resize(f, 0, 0, 1, 0) end),
    kpress("Down", function(f) ionframe_do_resize(f, 0, 0, 0, 1) end),
    kpress("F",    function(f) ionframe_do_resize(f, 1, 0, 0, 0) end),
    kpress("B",	   function(f) ionframe_do_resize(f, 0, 1, 0, 0) end),
    kpress("P",    function(f) ionframe_do_resize(f, 0, 0, 1, 0) end),
    kpress("N",    function(f) ionframe_do_resize(f, 0, 0, 0, 1) end),

    kpress("Shift+Left", function(f) ionframe_do_resize(f,-1, 0, 0, 0) end),
    kpress("Shift+Right",function(f) ionframe_do_resize(f, 0,-1, 0, 0) end),
    kpress("Shift+Up",   function(f) ionframe_do_resize(f, 0, 0,-1, 0) end),
    kpress("Shift+Down", function(f) ionframe_do_resize(f, 0, 0, 0,-1) end),
    kpress("Shift+F",    function(f) ionframe_do_resize(f,-1, 0, 0, 0) end),
    kpress("Shift+B",    function(f) ionframe_do_resize(f, 0,-1, 0, 0) end),
    kpress("Shift+P",    function(f) ionframe_do_resize(f, 0, 0,-1, 0) end),
    kpress("Shift+N",    function(f) ionframe_do_resize(f, 0, 0, 0,-1) end),
}
