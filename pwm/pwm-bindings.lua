-- 
-- Ion bindings configuration file. Global bindings and bindings common 
-- to screens and all types of frames only. See modules' configuration 
-- files for other bindings.
--


-- WScreen context bindings
--
-- The bindings in this context are available all the time.
--
-- The variable DEFAULT_MOD should contain a string of the form 'Mod1+'
-- where Mod1 maybe replaced with the modifier you want to use for most
-- of the bindings. Similarly SECOND_MOD may be redefined to add a 
-- modifier to some of the F-key bindings.

defbindings("WScreen", {
    kpress(DEFAULT_MOD.."1", "switch_nth", 0),
    kpress(DEFAULT_MOD.."2", "switch_nth", 1),
    kpress(DEFAULT_MOD.."3", "switch_nth", 2),
    kpress(DEFAULT_MOD.."4", "switch_nth", 3),
    kpress(DEFAULT_MOD.."5", "switch_nth", 4),
    kpress(DEFAULT_MOD.."6", "switch_nth", 5),
    kpress(DEFAULT_MOD.."7", "switch_nth", 6),
    kpress(DEFAULT_MOD.."8", "switch_nth", 7),
    kpress(DEFAULT_MOD.."9", "switch_nth", 8),
    kpress(DEFAULT_MOD.."0", "switch_nth", 9),
    kpress(DEFAULT_MOD.."Left", "switch_prev"),
    kpress(DEFAULT_MOD.."Right", "switch_next"),
    
    submap(DEFAULT_MOD.."K", {
        kpress("AnyModifier+K", "goto_previous"),
        kpress("AnyModifier+T", "clear_tags"),
    }),
    
    kpress(DEFAULT_MOD.."Shift+1", "goto_nth_screen", 0),
    kpress(DEFAULT_MOD.."Shift+2", "goto_nth_screen", 1),
    kpress(DEFAULT_MOD.."Shift+Left", "goto_next_screen"),
    kpress(DEFAULT_MOD.."Shift+Right", "goto_prev_screen"),
    
    kpress(DEFAULT_MOD.."F1", "show_manual", "pwm"),
    kpress("F2", "exec", "xterm"),
    
    kpress(DEFAULT_MOD.."F9", "create_new_ws"),

    kpress(SECOND_MOD..KEYF12, "bigmenu", "mainmenu"),
    mpress("Button2", "pmenu", "windowlist"),
    mpress("Button3", "pmenu", "mainmenu"),
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
    kpress_waitrel(DEFAULT_MOD.."C", "close_sub_or_self"),
    kpress_waitrel(DEFAULT_MOD.."L", "@sub_cwin", "broken_app_resize_kludge"),
    kpress_waitrel(DEFAULT_MOD.."Return", "@sub_cwin", "toggle_fullscreen"),

    submap(DEFAULT_MOD.."K", {
        kpress("AnyModifier+C", "@sub_cwin", "kill"),
        kpress("AnyModifier+Q", "@sub_cwin", "quote_next"),
    }),
})


-- WFrame context bindings
--
-- These bindings are common to all types of frames. The rest of frame
-- bindings that differ between frame types are defined in the modules' 
-- configuration files.

defbindings("WFrame", {
    -- Tag viewed object
    kpress(DEFAULT_MOD.."T", "@sub", "toggle_tag"),

    submap(DEFAULT_MOD.."K", {
        -- Selected object/tab switching
        kpress("AnyModifier+N", "switch_next"),
        kpress("AnyModifier+P", "switch_prev"),
        kpress("AnyModifier+1", "switch_nth", 0),
        kpress("AnyModifier+2", "switch_nth", 1),
        kpress("AnyModifier+3", "switch_nth", 2),
        kpress("AnyModifier+4", "switch_nth", 3),
        kpress("AnyModifier+5", "switch_nth", 4),
        kpress("AnyModifier+6", "switch_nth", 5),
        kpress("AnyModifier+7", "switch_nth", 6),
        kpress("AnyModifier+8", "switch_nth", 7),
        kpress("AnyModifier+9", "switch_nth", 8),
        kpress("AnyModifier+0", "switch_nth", 9),
        kpress("Left", "dec_managed_index"),
        kpress("Right", "inc_managed_index"),
        -- Maximize
        kpress("AnyModifier+H", "maximize_horiz"),
        kpress("AnyModifier+V", "maximize_vert"),
        -- Attach tagged objects
        kpress("AnyModifier+A", "attach_tagged"),
    }),
    
    -- Queries
    --[[
    kpress(DEFAULT_MOD.."A", "query_attachclient"),
    kpress(DEFAULT_MOD.."G", "query_gotoclient"),
    kpress(SECOND_MOD.."F1", "query_man"),
    kpress(SECOND_MOD.."F3", "query_exec"),
    kpress(DEFAULT_MOD.."F3", "query_lua"),
    kpress(SECOND_MOD.."F4", "query_ssh"),
    kpress(SECOND_MOD.."F5", "query_editfile"),
    kpress(SECOND_MOD.."F6", "query_runfile"),
    kpress(SECOND_MOD.."F9", "query_workspace"),
    --]]

    -- Menus
    kpress(DEFAULT_MOD.."M", "menu", "ctxmenu"),
    mpress("Button3", "pmenu", "ctxmenu"),
    
    -- Move/resize mode
    kpress(DEFAULT_MOD.."R", "begin_moveres"),
    
    -- Pointing device
    mclick("Button1@tab", "p_switch_tab"),
    mdrag("Button1@tab", "p_tabdrag"),
    mdrag("Button1@border", "p_resize"),
    
    mclick("Button2@tab", "p_switch_tab"),
    mdrag("Button2@tab", "p_tabdrag"),
    
    mdrag(DEFAULT_MOD.."Button3", "p_resize"),
})


-- WMoveresMode context bindings
-- 
-- These bindings are available keyboard move/resize mode. The mode
-- is activated on frames with the command begin_moveres (bound to
-- DEFAULT_MOD.."R" above by default).

defbindings("WMoveresMode", {
    kpress("AnyModifier+Escape", "cancel"),
    kpress("AnyModifier+Return", "end"),
    
    kpress("Left", "resize", 1, 0, 0, 0),
    kpress("Right","resize", 0, 1, 0, 0),
    kpress("Up",   "resize", 0, 0, 1, 0),
    kpress("Down", "resize", 0, 0, 0, 1),
    kpress("F",    "resize", 1, 0, 0, 0),
    kpress("B",	   "resize", 0, 1, 0, 0),
    kpress("P",    "resize", 0, 0, 1, 0),
    kpress("N",    "resize", 0, 0, 0, 1),

    kpress("Shift+Left", "resize",-1, 0, 0, 0),
    kpress("Shift+Right","resize", 0,-1, 0, 0),
    kpress("Shift+Up",   "resize", 0, 0,-1, 0),
    kpress("Shift+Down", "resize", 0, 0, 0,-1),
    kpress("Shift+F",    "resize",-1, 0, 0, 0),
    kpress("Shift+B",    "resize", 0,-1, 0, 0),
    kpress("Shift+P",    "resize", 0, 0,-1, 0),
    kpress("Shift+N",    "resize", 0, 0, 0,-1),

    kpress(DEFAULT_MOD.."Left", "move",-1, 0),
    kpress(DEFAULT_MOD.."Right","move", 1, 0),
    kpress(DEFAULT_MOD.."Up", 	"move", 0,-1),
    kpress(DEFAULT_MOD.."Down", "move", 0, 1),
    kpress(DEFAULT_MOD.."F",    "move",-1, 0),
    kpress(DEFAULT_MOD.."B",    "move", 1, 0),
    kpress(DEFAULT_MOD.."P", 	"move", 0,-1),
    kpress(DEFAULT_MOD.."N",    "move", 0, 1),
})

