--
-- Ion floatws module configuration file
--

-- Bindings for PWM-style floating frame workspaces. These should work
-- on any object on the workspace.

floatws_bindings{
    kpress(DEFAULT_MOD .. "Tab",
           function(ws) region_raise(floatws_circulate(ws)) end),
    submap(DEFAULT_MOD .. "K") {
        kpress("AnyModifier+Tab",
               function(ws) region_raise(floatws_backcirculate(ws)) end),
    },
}


-- Frame bindings. These work in (floating/PWM-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ioncore-bindings.lua.

floatframe_bindings{
    kpress(DEFAULT_MOD .. "R", floatframe_begin_resize),
    
    mpress("Button1", region_raise, "tab"),
    mpress("Button1", region_raise, "border"),
    mclick("Button1", genframe_p_switch_tab, "tab"),
    mdrag("Button1", floatframe_p_move, "tab"),
    mdrag("Button1", genframe_p_resize, "border"),
    mdblclick("Button1", floatframe_toggle_shade, "tab"),
    
    mclick(DEFAULT_MOD .. "Button1", region_raise),
    mdrag(DEFAULT_MOD .. "Button1", floatframe_p_move),
    
    mclick("Button2", genframe_p_switch_tab, "tab"),
    mdrag("Button2", genframe_p_tabdrag, "tab"),
    
    mclick(DEFAULT_MOD .. "Button3", region_lower),
    mdrag(DEFAULT_MOD .. "Button3", genframe_p_resize),
}


-- Frame move/resize mode bindings

floatframe_moveres_bindings{
    kpress("AnyModifier+Escape", floatframe_cancel_resize),
    kpress("AnyModifier+Return", floatframe_end_resize),
    
    kpress("Left", function(f) floatframe_do_resize(f, 1, 0, 0, 0) end),
    kpress("Right",function(f) floatframe_do_resize(f, 0, 1, 0, 0) end),
    kpress("Up",   function(f) floatframe_do_resize(f, 0, 0, 1, 0) end),
    kpress("Down", function(f) floatframe_do_resize(f, 0, 0, 0, 1) end),
    kpress("F",    function(f) floatframe_do_resize(f, 1, 0, 0, 0) end),
    kpress("B",	   function(f) floatframe_do_resize(f, 0, 1, 0, 0) end),
    kpress("P",    function(f) floatframe_do_resize(f, 0, 0, 1, 0) end),
    kpress("N",    function(f) floatframe_do_resize(f, 0, 0, 0, 1) end),

    kpress("Shift+Left", function(f) floatframe_do_resize(f,-1, 0, 0, 0) end),
    kpress("Shift+Right",function(f) floatframe_do_resize(f, 0,-1, 0, 0) end),
    kpress("Shift+Up",   function(f) floatframe_do_resize(f, 0, 0,-1, 0) end),
    kpress("Shift+Down", function(f) floatframe_do_resize(f, 0, 0, 0,-1) end),
    kpress("Shift+F",    function(f) floatframe_do_resize(f,-1, 0, 0, 0) end),
    kpress("Shift+B",    function(f) floatframe_do_resize(f, 0,-1, 0, 0) end),
    kpress("Shift+P",    function(f) floatframe_do_resize(f, 0, 0,-1, 0) end),
    kpress("Shift+N",    function(f) floatframe_do_resize(f, 0, 0, 0,-1) end),

    kpress(DEFAULT_MOD.."Left", function(f) floatframe_do_move(f,-1, 0) end),
    kpress(DEFAULT_MOD.."Right",function(f) floatframe_do_move(f, 1, 0) end),
    kpress(DEFAULT_MOD.."Up", 	function(f) floatframe_do_move(f, 0,-1) end),
    kpress(DEFAULT_MOD.."Down", function(f) floatframe_do_move(f, 0, 1) end),
    kpress(DEFAULT_MOD.."F",    function(f) floatframe_do_move(f,-1, 0) end),
    kpress(DEFAULT_MOD.."B",    function(f) floatframe_do_move(f, 1, 0) end),
    kpress(DEFAULT_MOD.."P", 	function(f) floatframe_do_move(f, 0,-1) end),
    kpress(DEFAULT_MOD.."N",    function(f) floatframe_do_move(f, 0, 1) end),
}

