-- 
-- Ion ioncore bindings configuration file. Global bindings and bindings
-- common to screens and all types of frames only. See modules'
-- configuration files for other bindings.
--


-- global_bindings {{{

-- Global_bindings are available all the time. The functions given here 
-- should accept WScreens as parameter. 

-- The variable DEFAULT_MOD should contain a string of the form 'Mod1+'
-- where Mod1 maybe replaced with the modifier you want to use for most
-- of the bindings. Similarly SECOND_MOD may be redefined to add a 
-- modifier to some of the F-key bindings.

global_bindings{
    kpress(DEFAULT_MOD.."1", function(s) s:switch_nth(0) end),
    kpress(DEFAULT_MOD.."2", function(s) s:switch_nth(1) end),
    kpress(DEFAULT_MOD.."3", function(s) s:switch_nth(2) end),
    kpress(DEFAULT_MOD.."4", function(s) s:switch_nth(3) end),
    kpress(DEFAULT_MOD.."5", function(s) s:switch_nth(4) end),
    kpress(DEFAULT_MOD.."6", function(s) s:switch_nth(5) end),
    kpress(DEFAULT_MOD.."7", function(s) s:switch_nth(6) end),
    kpress(DEFAULT_MOD.."8", function(s) s:switch_nth(7) end),
    kpress(DEFAULT_MOD.."9", function(s) s:switch_nth(8) end),
    kpress(DEFAULT_MOD.."0", function(s) s:switch_nth(9) end),
    kpress(DEFAULT_MOD.."Left", WScreen.switch_prev),
    kpress(DEFAULT_MOD.."Right", WScreen.switch_next),
    
    submap(DEFAULT_MOD.."K") {
        kpress("AnyModifier+K", goto_previous),
        kpress("AnyModifier+T", clear_tags),
    },
    
    kpress(DEFAULT_MOD.."Shift+1", function() goto_nth_screen(0) end),
    kpress(DEFAULT_MOD.."Shift+2", function() goto_nth_screen(1) end),
    kpress(DEFAULT_MOD.."Shift+Left", goto_next_screen),
    kpress(DEFAULT_MOD.."Shift+Right", goto_prev_screen),
    
    kpress(DEFAULT_MOD.."F1", function(scr)
                                  -- Display Ion's manual page
                                  local m=lookup_script("ion-man")
                                  if m then
                                      exec_in(scr, m.." ion")
                                  end
                              end),
    kpress("F2", make_exec_fn("xterm")),
    
    -- Create a new workspace with a default name.
    kpress(DEFAULT_MOD.."F9", 
           function(scr)
               scr:attach_new({ type=default_ws_type, switchto=true })
           end),
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
--     local reg=mplex:current()
--     if obj_is(reg, "WClientWin") then 
--         fn(reg)
--     end
-- end
-- 
-- For details see the document "Ion: Configuring and extending with Lua".

mplex_bindings{
    kpress_waitrel(DEFAULT_MOD.."C", WRegion.close_sub_or_self),
    kpress_waitrel(DEFAULT_MOD.."L", 
                   make_current_clientwin_fn(WClientWin.broken_app_resize_kludge)),
    kpress_waitrel(DEFAULT_MOD.."Return", 
                   make_current_clientwin_fn(WClientWin.toggle_fullscreen)),

    submap(DEFAULT_MOD.."K") {
        kpress("AnyModifier+C",
               make_current_clientwin_fn(WClientWin.kill)),
        kpress("AnyModifier+Q", 
               make_current_clientwin_fn(WClientWin.quote_next)),
    },
    
}

-- }}}


-- genframe_bindings {{{

-- These bindings are common to all types of frames. The rest of frame
-- bindings that differ between frame types are defined in the modules' 
-- configuration files.

genframe_bindings{
    -- Tag viewed object
    kpress(DEFAULT_MOD.."T", make_current_fn(WRegion.toggle_tag)),

    submap(DEFAULT_MOD.."K") {
        -- Selected object/tab switching
        kpress("AnyModifier+N", WGenFrame.switch_next),
        kpress("AnyModifier+P", WGenFrame.switch_prev),
        kpress("AnyModifier+1", function(f) f:switch_nth(0) end),
        kpress("AnyModifier+2", function(f) f:switch_nth(1) end),
        kpress("AnyModifier+3", function(f) f:switch_nth(2) end),
        kpress("AnyModifier+4", function(f) f:switch_nth(3) end),
        kpress("AnyModifier+5", function(f) f:switch_nth(4) end),
        kpress("AnyModifier+6", function(f) f:switch_nth(5) end),
        kpress("AnyModifier+7", function(f) f:switch_nth(6) end),
        kpress("AnyModifier+8", function(f) f:switch_nth(7) end),
        kpress("AnyModifier+9", function(f) f:switch_nth(8) end),
        kpress("AnyModifier+0", function(f) f:switch_nth(9) end),
        -- Maximize
        kpress("AnyModifier+H", WGenFrame.maximize_horiz),
        kpress("AnyModifier+V", WGenFrame.maximize_vert),
        -- Attach tagged objects
        kpress("AnyModifier+A", WGenFrame.attach_tagged),
    },
}

-- }}}


-- Queries {{{

-- The bindings that pop up queries are defined here. The bindings to edit
-- text in queries etc. are defined in the query module's configuration file
-- query.lua. If you are not going to load the query module, you might as
-- well comment out the following include statement. This should free up 
-- some memory and prevent non-working bindings from being defined.

include("querylib.lua")

if querylib then
    local f11key, f12key="F11", "F12"
    
    -- If we're on SunOS, we need to remap some keys.
    if os.execute('uname -s -p|grep "SunOS sparc" > /dev/null')==0 then
        print("ioncore-bindings.lua: Uname test reported SunOS; ".. 
              "mapping F11=Sun36, F12=SunF37.")
        f11key, f12key="SunF36", "SunF37"
    end
    
    -- Frame-level queries
    genframe_bindings{
        kpress(DEFAULT_MOD.."A", querylib.query_attachclient),
        kpress(DEFAULT_MOD.."G", querylib.query_gotoclient),
        
        kpress(SECOND_MOD.."F1", querylib.query_man),
        kpress(SECOND_MOD.."F3", querylib.query_exec),
        kpress(DEFAULT_MOD.."F3", querylib.query_lua),
        kpress(SECOND_MOD.."F4", querylib.query_ssh),
        kpress(SECOND_MOD.."F5", querylib.query_editfile),
        kpress(SECOND_MOD.."F6", querylib.query_runfile),
        kpress(SECOND_MOD.."F9", querylib.query_workspace),
    }
    
    -- Screen-level queries. Queries generally appear in frames to be 
    -- consistent although do not affect the frame, but these two are
    -- special.
    global_bindings{
        kpress(SECOND_MOD..f11key, querylib.query_restart),
        kpress(SECOND_MOD..f12key, querylib.query_exit),
    }
end

-- }}}
