--
-- Ion ionws module configuration file
--

-- Bindings for the tiled workspaces (ionws). These should work on any 
-- object on the workspace.

ionws_bindings{
    kpress(DEFAULT_MOD .. "N", WIonWS.goto_below),
    kpress(DEFAULT_MOD .. "P", WIonWS.goto_above),
    kpress(DEFAULT_MOD .. "Tab", WIonWS.goto_right),
    submap(DEFAULT_MOD .. "K", {
        kpress("AnyModifier+Tab", WIonWS.goto_left),
    }),
}


-- Frame bindings. These work in (Ion/tiled-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ioncore-bindings.lua.

ionframe_bindings{
    kpress(DEFAULT_MOD .. "R", WIonFrame.begin_resize),
    kpress(DEFAULT_MOD .. "S",
           function(frame) frame:split("bottom") end),

    submap(DEFAULT_MOD .. "K", {
        kpress("AnyModifier+X", WIonFrame.relocate_and_close),
        kpress("AnyModifier+S",
               function(frame) frame:split("right") end),
    }),
    
    mclick("Button1", WGenFrame.p_switch_tab, "tab"),
    mdblclick("Button1", WIonFrame.toggle_shade, "tab"),
    mdrag("Button1", WGenFrame.p_tabdrag, "tab"),
    mdrag("Button1", WGenFrame.p_resize, "border"),
    
    mclick("Button2", WGenFrame.p_switch_tab, "tab"),
    mdrag("Button2", WGenFrame.p_tabdrag, "tab"),
    
    mdrag(DEFAULT_MOD .. "Button3", WGenFrame.p_resize),
}


-- Frame resize mode bindings

ionframe_moveres_bindings{
    kpress("AnyModifier+Escape", WIonFrame.cancel_resize),
    kpress("AnyModifier+Return", WIonFrame.end_resize),
    
    kpress("Left", function(f) f:do_resize( 1, 0, 0, 0) end),
    kpress("Right",function(f) f:do_resize( 0, 1, 0, 0) end),
    kpress("Up",   function(f) f:do_resize( 0, 0, 1, 0) end),
    kpress("Down", function(f) f:do_resize( 0, 0, 0, 1) end),
    kpress("F",    function(f) f:do_resize( 1, 0, 0, 0) end),
    kpress("B",	   function(f) f:do_resize( 0, 1, 0, 0) end),
    kpress("P",    function(f) f:do_resize( 0, 0, 1, 0) end),
    kpress("N",    function(f) f:do_resize( 0, 0, 0, 1) end),

    kpress("Shift+Left", function(f) f:do_resize(-1, 0, 0, 0) end),
    kpress("Shift+Right",function(f) f:do_resize( 0,-1, 0, 0) end),
    kpress("Shift+Up",   function(f) f:do_resize( 0, 0,-1, 0) end),
    kpress("Shift+Down", function(f) f:do_resize( 0, 0, 0,-1) end),
    kpress("Shift+F",    function(f) f:do_resize(-1, 0, 0, 0) end),
    kpress("Shift+B",    function(f) f:do_resize( 0,-1, 0, 0) end),
    kpress("Shift+P",    function(f) f:do_resize( 0, 0,-1, 0) end),
    kpress("Shift+N",    function(f) f:do_resize( 0, 0, 0,-1) end),
}
