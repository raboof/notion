--
-- Ion ioncore bindings configuration file. Global bindings only, see
-- modules' configuration files for other bindings.
--


global_bindings{
    kpress(DEFAULT_MOD .. "1", function(s) screen_switch_nth_on_cvp(s, 0) end),
    kpress(DEFAULT_MOD .. "2", function(s) screen_switch_nth_on_cvp(s, 1) end),
    kpress(DEFAULT_MOD .. "3", function(s) screen_switch_nth_on_cvp(s, 2) end),
    kpress(DEFAULT_MOD .. "4", function(s) screen_switch_nth_on_cvp(s, 3) end),
    kpress(DEFAULT_MOD .. "5", function(s) screen_switch_nth_on_cvp(s, 4) end),
    kpress(DEFAULT_MOD .. "6", function(s) screen_switch_nth_on_cvp(s, 5) end),
    kpress(DEFAULT_MOD .. "7", function(s) screen_switch_nth_on_cvp(s, 6) end),
    kpress(DEFAULT_MOD .. "8", function(s) screen_switch_nth_on_cvp(s, 7) end),
    kpress(DEFAULT_MOD .. "9", function(s) screen_switch_nth_on_cvp(s, 8) end),
    kpress(DEFAULT_MOD .. "0", function(s) screen_switch_nth_on_cvp(s, 9) end),
    kpress(DEFAULT_MOD .. "Left", screen_switch_prev_on_cvp),
    kpress(DEFAULT_MOD .. "Right", screen_switch_next_on_cvp),
    
    -- make_active_leaf_fn(fn) (defined in ioncore-lib.lua) creates a
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


