-- 
-- Ion ioncore bindings configuration file. Global bindings and bindings
-- common to screens and all types of frames only. See modules'
-- configuration files for other bindings.
--


-- global_bindings {{{

-- Global_bindings are available all the time. The functions given here 
-- should accept WScreens as parameter.

global_bindings{
    kpress(DEFAULT_MOD .. "1", function(s) screen_switch_nth(s, 0) end),
    kpress(DEFAULT_MOD .. "2", function(s) screen_switch_nth(s, 1) end),
    kpress(DEFAULT_MOD .. "3", function(s) screen_switch_nth(s, 2) end),
    kpress(DEFAULT_MOD .. "4", function(s) screen_switch_nth(s, 3) end),
    kpress(DEFAULT_MOD .. "5", function(s) screen_switch_nth(s, 4) end),
    kpress(DEFAULT_MOD .. "6", function(s) screen_switch_nth(s, 5) end),
    kpress(DEFAULT_MOD .. "7", function(s) screen_switch_nth(s, 6) end),
    kpress(DEFAULT_MOD .. "8", function(s) screen_switch_nth(s, 7) end),
    kpress(DEFAULT_MOD .. "9", function(s) screen_switch_nth(s, 8) end),
    kpress(DEFAULT_MOD .. "0", function(s) screen_switch_nth(s, 9) end),
    kpress(DEFAULT_MOD .. "Left", screen_switch_prev),
    kpress(DEFAULT_MOD .. "Right", screen_switch_next),
    
    submap(DEFAULT_MOD .. "K") {
        kpress("AnyModifier+K", goto_previous),
    },
    
    kpress(DEFAULT_MOD .. "Shift+1", function() goto_nth_screen(0) end),
    kpress(DEFAULT_MOD .. "Shift+2", function() goto_nth_screen(1) end),
    kpress(DEFAULT_MOD .. "Shift+Left", goto_next_screen),
    kpress(DEFAULT_MOD .. "Shift+Right", goto_prev_screen),
    
    kpress(DEFAULT_MOD .. "F1", make_exec_fn("ion-man ion")),
    kpress("F2", make_exec_fn("xterm")),
    
    -- Create a new workspace with a default name.
    kpress(DEFAULT_MOD .. "F9", 
           function(scr)
               local r=region_manage_new(scr, {type=default_ws_type})
               if r then region_goto(r) end
           end)
}

-- }}}


-- mplex_bindings {{{

-- These bindings work in frames and on screens. The innermost of such
-- objects always gets to handle the key press. Essentially these bindings
-- are used to define actions on client windows. (Remember that client
-- windows can be put in fullscreen mode and therefore may not have a
-- frame.)
-- 
-- The make_*_fn functions are used to call functions on the object currently 
-- managed by the screen or frame or the frame itself. Essentially e.g.
-- make_current_clientwin_fn(fn) expands to
-- 
-- function(mplex)
--     local reg=mplex_current(mplex)
--     if obj_is(reg, "WClientWin") then 
--         fn(reg)
--     end
-- end
-- 
-- For details see the document "Ion: Configuring and extending with Lua".

mplex_bindings{
    kpress_waitrel(DEFAULT_MOD .. "C", close_sub_or_self),
    kpress_waitrel(DEFAULT_MOD .. "L", 
                   make_current_clientwin_fn(clientwin_broken_app_resize_kludge)),
    kpress_waitrel(DEFAULT_MOD .. "Return", 
                   make_current_clientwin_fn(clientwin_toggle_fullscreen)),

    submap(DEFAULT_MOD .. "K") {
        kpress("AnyModifier+C",
               make_current_clientwin_fn(clientwin_kill)),
        kpress("AnyModifier+Q", 
               make_current_clientwin_fn(clientwin_quote_next)),
    }
}

-- }}}


-- genframe_bindings {{{

-- These bindings are common to all types of frames. The rest of frame
-- bindings that differ between frame types are defined in the modules' 
-- configuration files.

genframe_bindings{
    submap(DEFAULT_MOD.."K") {
            kpress("AnyModifier+N", genframe_switch_next),
            kpress("AnyModifier+P", genframe_switch_prev),
            kpress("AnyModifier+1", 
                   function(f) genframe_switch_nth(f, 0) end),
            kpress("AnyModifier+2",
                   function(f) genframe_switch_nth(f, 1) end),
            kpress("AnyModifier+3",
                   function(f) genframe_switch_nth(f, 2) end),
            kpress("AnyModifier+4",
                   function(f) genframe_switch_nth(f, 3) end),
            kpress("AnyModifier+5",
                   function(f) genframe_switch_nth(f, 4) end),
            kpress("AnyModifier+6",
                   function(f) genframe_switch_nth(f, 5) end),
            kpress("AnyModifier+7",
                   function(f) genframe_switch_nth(f, 6) end),
            kpress("AnyModifier+8",
                   function(f) genframe_switch_nth(f, 7) end),
            kpress("AnyModifier+9",
                   function(f) genframe_switch_nth(f, 8) end),
            kpress("AnyModifier+0",
                   function(f) genframe_switch_nth(f, 9) end),
            kpress("AnyModifier+H", genframe_maximize_horiz),
            kpress("AnyModifier+V", genframe_maximize_vert),
    }
}

-- }}}


-- Queries {{{

-- The bindings that pop up queries are defined here. The bindings to edit
-- text in queries etc. are defined in the query module's configuration file
-- query.lua. If you are not going to load the query module, you might as
-- well comment out the following include statement. This should free up 
-- some memory and prevent non-working bindings from being defined.

include("querylib.lua")

if QueryLib then
    local f11key, f12key="F11", "F12"
    
    -- If we're on SunOS, we need to remap some keys.
    if os.execute('uname -s -p|grep "SunOS sparc" > /dev/null')==0 then
        print("ioncore-bindings.lua: Uname test reported SunOS; ".. 
              "mapping F11=Sun36, F12=SunF37.")
        f11key, f12key="SunF36", "SunF37"
    end
    
    -- Frame-level queries
    genframe_bindings{
        kpress(DEFAULT_MOD.."A", QueryLib.query_attachclient),
        kpress(DEFAULT_MOD.."G", QueryLib.query_gotoclient),
        
        kpress("F1", QueryLib.query_man),
        kpress("F3", QueryLib.query_exec),
        kpress(DEFAULT_MOD .. "F3", QueryLib.query_lua),
        kpress("F4", QueryLib.query_ssh),
        kpress("F5", QueryLib.query_editfile),
        kpress("F6", QueryLib.query_runfile),
        kpress("F9", QueryLib.query_workspace),
    }
    
    -- Screen-level queries. Queries generally appear in frames to be 
    -- consistent although do not affect the frame, but these two are
    -- special.
    global_bindings{
        kpress(f11key, QueryLib.query_restart),
        kpress(f12key, QueryLib.query_exit),
    }
end

-- }}}
