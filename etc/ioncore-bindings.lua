--
-- Ion ioncore bindings configuration file. Global bindings only, see
-- modules' configuration files for other bindings.
--

global_bindings{
    kpress(DEFAULT_MOD .. "1", function(scr) switch_ws_nth(scr, 0) end),
    kpress(DEFAULT_MOD .. "2", function(scr) switch_ws_nth(scr, 1) end),
    kpress(DEFAULT_MOD .. "3", function(scr) switch_ws_nth(scr, 2) end),
    kpress(DEFAULT_MOD .. "4", function(scr) switch_ws_nth(scr, 3) end),
    kpress(DEFAULT_MOD .. "5", function(scr) switch_ws_nth(scr, 4) end),
    kpress(DEFAULT_MOD .. "6", function(scr) switch_ws_nth(scr, 5) end),
    kpress(DEFAULT_MOD .. "7", function(scr) switch_ws_nth(scr, 6) end),
    kpress(DEFAULT_MOD .. "8", function(scr) switch_ws_nth(scr, 7) end),
    kpress(DEFAULT_MOD .. "9", function(scr) switch_ws_nth(scr, 8) end),
    kpress(DEFAULT_MOD .. "0", function(scr) switch_ws_nth(scr, 9) end),
    kpress(DEFAULT_MOD .. "Left", switch_ws_prev),
    kpress(DEFAULT_MOD .. "Right", switch_ws_next),
    
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


