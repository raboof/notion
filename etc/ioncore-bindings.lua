--
-- Ion ioncore bindings configuration file. Global bindings only, see
-- modules' configuration files for other bindings.
--

global_bindings{
    -- make_screen_switch_nth_fn(n) (defined in ioncore-lib.lua) creates a
    -- function to call screen_switch_nth_fn_on_cvp(screen, n), where cvp
    -- stands for "current viewport" and screen is the parameter with which
    -- the key handling code in Ion calls us.
    kpress(DEFAULT_MOD .. "1", make_screen_switch_nth_fn(0)),
    kpress(DEFAULT_MOD .. "2", make_screen_switch_nth_fn(1)),
    kpress(DEFAULT_MOD .. "3", make_screen_switch_nth_fn(2)),
    kpress(DEFAULT_MOD .. "4", make_screen_switch_nth_fn(3)),
    kpress(DEFAULT_MOD .. "5", make_screen_switch_nth_fn(4)),
    kpress(DEFAULT_MOD .. "6", make_screen_switch_nth_fn(5)),
    kpress(DEFAULT_MOD .. "7", make_screen_switch_nth_fn(6)),
    kpress(DEFAULT_MOD .. "8", make_screen_switch_nth_fn(7)),
    kpress(DEFAULT_MOD .. "9", make_screen_switch_nth_fn(8)),
    kpress(DEFAULT_MOD .. "0", make_screen_switch_nth_fn(9)),
    kpress(DEFAULT_MOD .. "Left", screen_switch_prev_on_cvp),
    kpress(DEFAULT_MOD .. "Right", screen_switch_next_on_cvp),
    
    -- make_active_leaf_fn(fn) (defined again in ioncore-lib.lua) creates a
    -- function to call fn(region_get_active_leaf(screen)).
    -- Region_get_active_leaf is a function that will start from it's
    -- parameter (the screen in this case) and advance active_sub links in
    -- Ion's tree hierarchy of objects on the screen to find the most
    -- recently active object until there are no children. Usually this object
    -- will be a frame, client window or an input widget.
    kpress_waitrel(DEFAULT_MOD .. "C", make_active_leaf_fn(region_close)),
    kpress_waitrel(DEFAULT_MOD .. "L",
                   make_active_leaf_fn(clientwin_broken_app_resize_kludge)),
    kpress_waitrel(DEFAULT_MOD .. "Return",
                   make_active_leaf_fn(clientwin_toggle_fullscreen)),
    
    submap(DEFAULT_MOD .. "K") {
        kpress("AnyModifier+C", make_active_leaf_fn(clientwin_kill)),
        kpress("AnyModifier+Q", make_active_leaf_fn(clientwin_quote_next)),
        kpress("AnyModifier+K", goto_previous),
    },
    
    kpress(DEFAULT_MOD .. "Shift+1", function() goto_nth_viewport(0) end),
    kpress(DEFAULT_MOD .. "Shift+2", function() goto_nth_viewport(1) end),
    kpress(DEFAULT_MOD .. "Shift+Left", goto_next_viewport),
    kpress(DEFAULT_MOD .. "Shift+Right", goto_prev_viewport),
    
    kpress(DEFAULT_MOD .. "F1", make_exec_fn("ion-man ion")),
    kpress("F2", make_exec_fn("xterm")),
}


