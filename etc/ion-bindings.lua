-- 
-- Ion bindings configuration file. Global bindings and bindings common 
-- to screens and all types of frames only. See modules' configuration 
-- files for other bindings.
--


-- WScreen context bindings
--
-- The bindings in this context are available all the time.
--
-- The variable MOD1 should contain a string of the form 'Mod1+'
-- where Mod1 maybe replaced with the modifier you want to use for most
-- of the bindings. Similarly MOD2 may be redefined to add a 
-- modifier to some of the F-key bindings.

defbindings("WScreen", {
    kpress(MOD1.."1",           "WScreen.switch_nth(_, 0)"),
    kpress(MOD1.."2",           "WScreen.switch_nth(_, 1)"),
    kpress(MOD1.."3",           "WScreen.switch_nth(_, 2)"),
    kpress(MOD1.."4",           "WScreen.switch_nth(_, 3)"),
    kpress(MOD1.."5",           "WScreen.switch_nth(_, 4)"),
    kpress(MOD1.."6",           "WScreen.switch_nth(_, 5)"),
    kpress(MOD1.."7",           "WScreen.switch_nth(_, 6)"),
    kpress(MOD1.."8",           "WScreen.switch_nth(_, 7)"),
    kpress(MOD1.."9",           "WScreen.switch_nth(_, 8)"),
    kpress(MOD1.."0",           "WScreen.switch_nth(_, 9)"),
    kpress(MOD1.."Left",        "WScreen.switch_prev(_)"),
    kpress(MOD1.."Right",       "WScreen.switch_next(_)"),
    
    submap(MOD1.."K", {
        kpress("AnyModifier+K", "ioncore.goto_previous()"),
        kpress("AnyModifier+T", "ioncore.clear_tags()"),
    }),
    
    kpress(MOD1.."Shift+1",     "ioncore.goto_nth_screen(0)"),
    kpress(MOD1.."Shift+2",     "ioncore.goto_nth_screen(1)"),
    kpress(MOD1.."Shift+Left",  "ioncore.goto_next_screen()"),
    kpress(MOD1.."Shift+Right", "ioncore.goto_prev_screen()"),
    
    kpress(MOD1.."F1",          "ioncorelib.show_manual('ion')"),
    kpress(MOD2.."F2",          "ioncore.exec('xterm')"),
    
    kpress(MOD1.."F9",          "ioncorelib.create_new_ws(_)"),
    
    kpress(MOD2.."F12",         "menulib.bigmenu(_, _sub, 'mainmenu')"),
    mpress("Button2",           "menulib.pmenu(_, _sub, 'windowlist')"),
    mpress("Button3",           "menulib.pmenu(_, _sub, 'mainmenu')"),
})


-- WMPlex context bindings
--
-- These bindings work in frames and on screens. The innermost of such
-- contexts/objects always gets to handle the key press. Essentially these 
-- bindings are used to define actions on client windows. (Remember that 
-- client windows can be put in fullscreen mode and therefore may not have a
-- frame.)
-- 
-- The @sub_cwin commands are used to call the parameter command on a
-- current client window managed by the multiplexer. For details see the 
-- document "Ion: Configuring and extending with Lua".

defbindings("WMPlex", {
    kpress_wait(MOD1.."C",      "WRegion.rqclose_propagate(_, _sub)"),
    kpress_wait(MOD1.."L",      "WClientWin.nudge(_sub)",
                                "_sub:WClientWin"),
   kpress_wait(MOD1.."Return",  "WClientWin.toggle_fullscreen(_sub)",
                                "_sub:WClientWin"),
                            
   submap(MOD1.."K", {
       kpress("AnyModifier+C",  "WClientWin.kill(_sub)", 
                                "_sub:WClientWin"),
       kpress("AnyModifier+Q",  "WClientWin.quote_next(_sub)",
                                "_sub:WClientWin"),
   }),
})


-- WFrame context bindings
--
-- These bindings are common to all types of frames. The rest of frame
-- bindings that differ between frame types are defined in the modules' 
-- configuration files.

defbindings("WFrame", {
    -- Tag viewed object
    kpress(MOD1.."T",           "WRegion.toggle_tag(_sub)",
                                "_sub:non-nil"),
           
    submap(MOD1.."K", {
        -- Selected object/tab switching
        kpress("AnyModifier+N", "WFrame.switch_next(_)"),
        kpress("AnyModifier+P", "WFrame.switch_prev(_)"),
        kpress("AnyModifier+1", "WFrame.switch_nth(_, 0)"),
        kpress("AnyModifier+2", "WFrame.switch_nth(_, 1)"),
        kpress("AnyModifier+3", "WFrame.switch_nth(_, 2)"),
        kpress("AnyModifier+4", "WFrame.switch_nth(_, 3)"),
        kpress("AnyModifier+5", "WFrame.switch_nth(_, 4)"),
        kpress("AnyModifier+6", "WFrame.switch_nth(_, 5)"),
        kpress("AnyModifier+7", "WFrame.switch_nth(_, 6)"),
        kpress("AnyModifier+8", "WFrame.switch_nth(_, 7)"),
        kpress("AnyModifier+9", "WFrame.switch_nth(_, 8)"),
        kpress("AnyModifier+0", "WFrame.switch_nth(_, 9)"),
        kpress("Left",          "WFrame.dec_managed_index(_, _sub)",
                                "_sub:non-nil"),
        kpress("Right",         "WFrame.inc_managed_index(_, _sub)",
                                "_sub:non-nil"),
        -- Maximize
        kpress("AnyModifier+H", "WFrame.maximize_horiz(_)"),
        kpress("AnyModifier+V", "WFrame.maximize_vert(_)"),
        -- Attach tagged objects
        kpress("AnyModifier+A", "WFrame.attach_tagged(_)"),
    }),
           
    -- Queries
    kpress(MOD1.."A",           "querylib.query_attachclient(_)"),
    kpress(MOD1.."G",           "querylib.query_gotoclient(_)"),
    kpress(MOD2.."F1",          "querylib.query_man(_)"),
    kpress(MOD2.."F3",          "querylib.query_exec(_)"),
    kpress(MOD1.."F3",          "querylib.query_lua(_)"),
    kpress(MOD2.."F4",          "querylib.query_ssh(_)"),
    kpress(MOD2.."F5",          "querylib.query_editfile(_)"),
    kpress(MOD2.."F6",          "querylib.query_runfile(_)"),
    kpress(MOD2.."F9",          "querylib.query_workspace(_)"),
           
    -- Menus
    kpress(MOD1.."M",           "menulib.menu(_, _sub, 'ctxmenu')"),
    mpress("Button3",           "menulib.pmenu(_, _sub, 'ctxmenu')"),
    
    -- Move/resize mode
    kpress(MOD1.."R",           "WFrame.begin_moveres(_)"),
    
    -- Pointing device
    mclick("Button1@tab",       "WFrame.p_switch_tab(_)"),
    mclick("Button2@tab",       "WFrame.p_switch_tab(_)"),
    mdrag("Button1@border",     "WFrame.p_resize(_)"),
    mdrag(MOD1.."Button3",      "WFrame.p_resize(_)"),
    mdrag("Button1@tab",        "WFrame.p_tabdrag(_)"),
    mdrag("Button2@tab",        "WFrame.p_tabdrag(_)"),
           
})


-- WMoveresMode context bindings
-- 
-- These bindings are available keyboard move/resize mode. The mode
-- is activated on frames with the command begin_moveres (bound to
-- MOD1.."R" above by default).

defbindings("WMoveresMode", {
    kpress("AnyModifier+Escape","WMoveresMode.cancel(_)"),
    kpress("AnyModifier+Return","WMoveresMode.finish(_)"),
    
    kpress("Left",              "WMoveresMode.resize(_, 1, 0, 0, 0)"),
    kpress("Right",             "WMoveresMode.resize(_, 0, 1, 0, 0)"),
    kpress("Up",                "WMoveresMode.resize(_, 0, 0, 1, 0)"),
    kpress("Down",              "WMoveresMode.resize(_, 0, 0, 0, 1)"),
    kpress("F",                 "WMoveresMode.resize(_, 1, 0, 0, 0)"),
    kpress("B",                 "WMoveresMode.resize(_, 0, 1, 0, 0)"),
    kpress("P",                 "WMoveresMode.resize(_, 0, 0, 1, 0)"),
    kpress("N",                 "WMoveresMode.resize(_, 0, 0, 0, 1)"),
    
    kpress("Shift+Left",        "WMoveresMode.resize(_,-1, 0, 0, 0)"),
    kpress("Shift+Right",       "WMoveresMode.resize(_, 0,-1, 0, 0)"),
    kpress("Shift+Up",          "WMoveresMode.resize(_, 0, 0,-1, 0)"),
    kpress("Shift+Down",        "WMoveresMode.resize(_, 0, 0, 0,-1)"),
    kpress("Shift+F",           "WMoveresMode.resize(_,-1, 0, 0, 0)"),
    kpress("Shift+B",           "WMoveresMode.resize(_, 0,-1, 0, 0)"),
    kpress("Shift+P",           "WMoveresMode.resize(_, 0, 0,-1, 0)"),
    kpress("Shift+N",           "WMoveresMode.resize(_, 0, 0, 0,-1)"),
    
    kpress(MOD1.."Left",        "WMoveresMode.move(_,-1, 0)"),
    kpress(MOD1.."Right",       "WMoveresMode.move(_, 1, 0)"),
    kpress(MOD1.."Up",          "WMoveresMode.move(_, 0,-1)"),
    kpress(MOD1.."Down",        "WMoveresMode.move(_, 0, 1)"),
    kpress(MOD1.."F",           "WMoveresMode.move(_,-1, 0)"),
    kpress(MOD1.."B",           "WMoveresMode.move(_, 1, 0)"),
    kpress(MOD1.."P",           "WMoveresMode.move(_, 0,-1)"),
    kpress(MOD1.."N",           "WMoveresMode.move(_, 0, 1)"),
})

