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
    bdoc("Switch to n:th object within the screen."),
    kpress(MOD1.."1", "WScreen.switch_nth(_, 0)"),
    kpress(MOD1.."2", "WScreen.switch_nth(_, 1)"),
    kpress(MOD1.."3", "WScreen.switch_nth(_, 2)"),
    kpress(MOD1.."4", "WScreen.switch_nth(_, 3)"),
    kpress(MOD1.."5", "WScreen.switch_nth(_, 4)"),
    kpress(MOD1.."6", "WScreen.switch_nth(_, 5)"),
    kpress(MOD1.."7", "WScreen.switch_nth(_, 6)"),
    kpress(MOD1.."8", "WScreen.switch_nth(_, 7)"),
    kpress(MOD1.."9", "WScreen.switch_nth(_, 8)"),
    kpress(MOD1.."0", "WScreen.switch_nth(_, 9)"),
    
    bdoc("Switch to next/previous object within the screen."),
    kpress(MOD1.."Right", "WScreen.switch_next(_)"),
    kpress(MOD1.."Left", "WScreen.switch_prev(_)"),
    
    submap(MOD1.."K", {
        bdoc("Go to previous active object."),
        kpress("AnyModifier+K", "ioncore.goto_previous()"),
        
        bdoc("Clear all tags."),
        kpress("AnyModifier+T", "ioncore.clear_tags()"),
    }),
    
    bdoc("Go to n:th screen."),
    kpress(MOD1.."Shift+1", "ioncore.goto_nth_screen(0)"),
    kpress(MOD1.."Shift+2", "ioncore.goto_nth_screen(1)"),
    
    bdoc("Go to next/previous screen."),
    kpress(MOD1.."Shift+Left", "ioncore.goto_next_screen()"),
    kpress(MOD1.."Shift+Right", "ioncore.goto_prev_screen()"),
    
    bdoc("Show the Ion manual page."),
    kpress(MOD1.."F1", "ioncore.show_manual()"),
    
    bdoc("Run a terminal emulator."),
    kpress(MOD2.."F2", "ioncore.exec_on(_, 'xterm')"),
    
    bdoc("Create a workspace of chosen default type."),
    kpress(MOD1.."F9", "ioncore.create_ws(_)"),
    
    bdoc("Display the main menu."),
    kpress(MOD2.."F12", "mod_menu.bigmenu(_, _sub, 'mainmenu')"),
    mpress("Button3", "mod_menu.pmenu(_, _sub, 'mainmenu')"),
    
    bdoc("Display the window list menu."),
    mpress("Button2", "mod_menu.pmenu(_, _sub, 'windowlist')"),
})


-- WMPlex context bindings
--
-- These bindings work in frames and on screens. The innermost of such
-- contexts/objects always gets to handle the key press. Essentially these 
-- bindings are used to define actions on client windows. (Remember that 
-- client windows can be put in fullscreen mode and therefore may not have a
-- frame.)
-- 
-- The "_sub:WClientWin" guards are used to ensure that _sub is a client
-- window in order to stop Ion from executing the callback with an invalid
-- parameter if it is not and then complaining.

defbindings("WMPlex", {
    bdoc("Close active object."),
    kpress_wait(MOD1.."C", "WRegion.rqclose_propagate(_, _sub)"),
    
    bdoc("Nudge the client window. This might help with some programs' "..
        "resizing problems."),
    kpress_wait(MOD1.."L", 
                "WClientWin.nudge(_sub)", "_sub:WClientWin"),

    bdoc("Toggle fullscreen mode."),
    kpress_wait(MOD1.."Return", 
                "WClientWin.toggle_fullscreen(_sub)", "_sub:WClientWin"),
                            
    submap(MOD1.."K", {
       bdoc("Kill the client."),
       kpress("AnyModifier+C", 
              "WClientWin.kill(_sub)", "_sub:WClientWin"),
                                
       bdoc("Send next key press to client window. "..
           "Some programs may not allow this by default."),
       kpress("AnyModifier+Q", 
              "WClientWin.quote_next(_sub)", "_sub:WClientWin"),
    }),
    
    bdoc("Query for manual page to be displayed."),
    kpress(MOD2.."F1", "mod_query.query_man(_)"),
    
    bdoc("Query for command line to execute."),
    kpress(MOD2.."F3", "mod_query.query_exec(_)"),

    bdoc("Query for Lua code to execute."),
    kpress(MOD1.."F3", "mod_query.query_lua(_)"),

    bdoc("Query for host to connect to with SSH."),
    kpress(MOD2.."F4", "mod_query.query_ssh(_)"),

    bdoc("Query for file to edit."),
    kpress(MOD2.."F5", "mod_query.query_editfile(_)"),

    bdoc("Query for file to view."),
    kpress(MOD2.."F6", "mod_query.query_runfile(_)"),

    bdoc("Query for workspace to go to or create a new one."),
    kpress(MOD2.."F9", "mod_query.query_workspace(_)"),
})


-- WFrame context bindings
--
-- These bindings are common to all types of frames. The rest of frame
-- bindings that differ between frame types are defined in the modules' 
-- configuration files.

defbindings("WFrame", {
    bdoc("Tag current object within the frame."),
    kpress(MOD1.."T", "WRegion.toggle_tag(_sub)", "_sub:non-nil"),
           
    submap(MOD1.."K", {
        bdoc("Switch to n:th object within the frame."),
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
        
        bdoc("Switch to next/previous object within the frame."),
        kpress("AnyModifier+N", "WFrame.switch_next(_)"),
        kpress("AnyModifier+P", "WFrame.switch_prev(_)"),
        
        bdoc("Move current object within the frame left/right."),
        kpress("AnyModifier+Left",
               "WFrame.dec_index(_, _sub)", "_sub:non-nil"),
        kpress("AnyModifier+Right",
               "WFrame.inc_index(_, _sub)", "_sub:non-nil"),
               
        bdoc("Maximize the frame horizontally/vertically."),
        kpress("AnyModifier+H", "WFrame.maximize_horiz(_)"),
        kpress("AnyModifier+V", "WFrame.maximize_vert(_)"),

        bdoc("Attach tagged objects to this frame."),
        kpress("AnyModifier+A", "WFrame.attach_tagged(_)"),
    }),

    bdoc("Query for a client window to attach to this frame."),
    kpress(MOD1.."A", "mod_query.query_attachclient(_)"),

    bdoc("Query for a client window to go to."),
    kpress(MOD1.."G", "mod_query.query_gotoclient(_)"),
           
    bdoc("Display frame context menu."),
    kpress(MOD1.."M", "mod_menu.menu(_, _sub, 'ctxmenu')"),
    mpress("Button3", "mod_menu.pmenu(_, _sub, 'ctxmenu')"),
    
    bdoc("Begin move/resize mode."),
    kpress(MOD1.."R", "WFrame.begin_moveres(_)"),
    
    bdoc("Switch the frame to display the object indicated by the tab."),
    mclick("Button1@tab", "WFrame.p_switch_tab(_)"),
    mclick("Button2@tab", "WFrame.p_switch_tab(_)"),
    
    bdoc("Resize the frame."),
    mdrag("Button1@border", "WFrame.p_resize(_)"),
    mdrag(MOD1.."Button3", "WFrame.p_resize(_)"),
    
    bdoc("Move the frame."),
    mdrag(MOD1.."Button1", "WFrame.p_move(_)"),
    
    bdoc("Move objects between frames by dragging and dropping the tab."),
    mdrag("Button1@tab", "WFrame.p_tabdrag(_)"),
    mdrag("Button2@tab", "WFrame.p_tabdrag(_)"),
           
})


-- WMoveresMode context bindings
-- 
-- These bindings are available keyboard move/resize mode. The mode
-- is activated on frames with the command begin_moveres (bound to
-- MOD1.."R" above by default).

defbindings("WMoveresMode", {
    bdoc("Cancel the resize mode."),
    kpress("AnyModifier+Escape","WMoveresMode.cancel(_)"),

    bdoc("End the resize mode."),
    kpress("AnyModifier+Return","WMoveresMode.finish(_)"),

    bdoc("Grow in specified direction."),
    kpress("Left",  "WMoveresMode.resize(_, 1, 0, 0, 0)"),
    kpress("Right", "WMoveresMode.resize(_, 0, 1, 0, 0)"),
    kpress("Up",    "WMoveresMode.resize(_, 0, 0, 1, 0)"),
    kpress("Down",  "WMoveresMode.resize(_, 0, 0, 0, 1)"),
    kpress("F",     "WMoveresMode.resize(_, 1, 0, 0, 0)"),
    kpress("B",     "WMoveresMode.resize(_, 0, 1, 0, 0)"),
    kpress("P",     "WMoveresMode.resize(_, 0, 0, 1, 0)"),
    kpress("N",     "WMoveresMode.resize(_, 0, 0, 0, 1)"),
    
    bdoc("Shrink in specified direction."),
    kpress("Shift+Left",  "WMoveresMode.resize(_,-1, 0, 0, 0)"),
    kpress("Shift+Right", "WMoveresMode.resize(_, 0,-1, 0, 0)"),
    kpress("Shift+Up",    "WMoveresMode.resize(_, 0, 0,-1, 0)"),
    kpress("Shift+Down",  "WMoveresMode.resize(_, 0, 0, 0,-1)"),
    kpress("Shift+F",     "WMoveresMode.resize(_,-1, 0, 0, 0)"),
    kpress("Shift+B",     "WMoveresMode.resize(_, 0,-1, 0, 0)"),
    kpress("Shift+P",     "WMoveresMode.resize(_, 0, 0,-1, 0)"),
    kpress("Shift+N",     "WMoveresMode.resize(_, 0, 0, 0,-1)"),
    
    bdoc("Move in specified direction."),
    kpress(MOD1.."Left",  "WMoveresMode.move(_,-1, 0)"),
    kpress(MOD1.."Right", "WMoveresMode.move(_, 1, 0)"),
    kpress(MOD1.."Up",    "WMoveresMode.move(_, 0,-1)"),
    kpress(MOD1.."Down",  "WMoveresMode.move(_, 0, 1)"),
    kpress(MOD1.."F",     "WMoveresMode.move(_,-1, 0)"),
    kpress(MOD1.."B",     "WMoveresMode.move(_, 1, 0)"),
    kpress(MOD1.."P",     "WMoveresMode.move(_, 0,-1)"),
    kpress(MOD1.."N",     "WMoveresMode.move(_, 0, 1)"),
})

