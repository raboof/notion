--
-- Ion floatws module configuration file
--

-- Bindings for PWM-style floating frame workspaces. These should work
-- on any object on the workspace.

floatws_bindings{
    kpress(DEFAULT_MOD .. "Tab",
           function(ws) ws:circulate():raise() end),
    submap(DEFAULT_MOD .. "K") { 
        kpress("AnyModifier+Tab",
               function(ws) ws:backcirculate():raise() end),
    },
}


-- Frame bindings. These work in (floating/PWM-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ioncore-bindings.lua.

floatframe_bindings{
    kpress(DEFAULT_MOD .. "R", WFloatFrame.begin_resize),
    
    mpress("Button1", WRegion.raise, "tab"),
    mpress("Button1", WRegion.raise, "border"),
    mclick("Button1", WGenFrame.p_switch_tab, "tab"),
    mdrag("Button1", WFloatFrame.p_move, "tab"),
    mdrag("Button1", WGenFrame.p_resize, "border"),
    mdblclick("Button1", WFloatFrame.toggle_shade, "tab"),
    
    mclick(DEFAULT_MOD .. "Button1", WRegion.raise),
    mdrag(DEFAULT_MOD .. "Button1", WFloatFrame.p_move),
    
    mclick("Button2", WGenFrame.p_switch_tab, "tab"),
    mdrag("Button2", WGenFrame.p_tabdrag, "tab"),
    
    mclick(DEFAULT_MOD .. "Button3", WRegion.lower),
    mdrag(DEFAULT_MOD .. "Button3", WGenFrame.p_resize),
}


-- Frame move/resize mode bindings

floatframe_moveres_bindings{
    kpress("AnyModifier+Escape", WFloatFrame.cancel_resize),
    kpress("AnyModifier+Return", WFloatFrame.end_resize),
    
    kpress("Left", function(f) f:do_resize(1, 0, 0, 0) end),
    kpress("Right",function(f) f:do_resize(0, 1, 0, 0) end),
    kpress("Up",   function(f) f:do_resize(0, 0, 1, 0) end),
    kpress("Down", function(f) f:do_resize(0, 0, 0, 1) end),
    kpress("F",    function(f) f:do_resize(1, 0, 0, 0) end),
    kpress("B",	   function(f) f:do_resize(0, 1, 0, 0) end),
    kpress("P",    function(f) f:do_resize(0, 0, 1, 0) end),
    kpress("N",    function(f) f:do_resize(0, 0, 0, 1) end),

    kpress("Shift+Left", function(f) f:do_resize(-1, 0, 0, 0) end),
    kpress("Shift+Right",function(f) f:do_resize( 0,-1, 0, 0) end),
    kpress("Shift+Up",   function(f) f:do_resize( 0, 0,-1, 0) end),
    kpress("Shift+Down", function(f) f:do_resize( 0, 0, 0,-1) end),
    kpress("Shift+F",    function(f) f:do_resize(-1, 0, 0, 0) end),
    kpress("Shift+B",    function(f) f:do_resize( 0,-1, 0, 0) end),
    kpress("Shift+P",    function(f) f:do_resize( 0, 0,-1, 0) end),
    kpress("Shift+N",    function(f) f:do_resize( 0, 0, 0,-1) end),

    kpress(DEFAULT_MOD.."Left", function(f) f:do_move(-1, 0) end),
    kpress(DEFAULT_MOD.."Right",function(f) f:do_move( 1, 0) end),
    kpress(DEFAULT_MOD.."Up", 	function(f) f:do_move( 0,-1) end),
    kpress(DEFAULT_MOD.."Down", function(f) f:do_move( 0, 1) end),
    kpress(DEFAULT_MOD.."F",    function(f) f:do_move(-1, 0) end),
    kpress(DEFAULT_MOD.."B",    function(f) f:do_move( 1, 0) end),
    kpress(DEFAULT_MOD.."P", 	function(f) f:do_move( 0,-1) end),
    kpress(DEFAULT_MOD.."N",    function(f) f:do_move( 0, 1) end),
}

