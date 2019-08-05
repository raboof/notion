--
-- Notion core configuration file
--


--
-- Bindings. This includes global bindings and bindings common to
-- screens and all types of frames only. See modules' configuration
-- files for other bindings.
--


-- WScreen context bindings
--
-- The bindings in this context are available all the time.
--
-- The variable META should contain a string of the form 'Mod4+'
-- where Mod4 maybe replaced with the modifier you want to use for most
-- of the bindings. Similarly ALTMETA may be redefined to add a
-- modifier to some of the F-key bindings.

defbindings("WScreen", {
    bdoc("Switch to object 0 (workspace, full screen client window) "..
         "within current screen.", "ws 0"),
    kpress(META.."1", "WScreen.switch_nth(_, 0)"),
    bdoc("Switch to object 1 (workspace, full screen client window) "..
         "within current screen.", "ws 1"),
    kpress(META.."2", "WScreen.switch_nth(_, 1)"),
    bdoc("Switch to object 2 (workspace, full screen client window) "..
         "within current screen.", "ws 2"),
    kpress(META.."3", "WScreen.switch_nth(_, 2)"),
    bdoc("Switch to object 3 (workspace, full screen client window) "..
         "within current screen.", "ws 3"),
    kpress(META.."4", "WScreen.switch_nth(_, 3)"),
    bdoc("Switch to object 4 (workspace, full screen client window) "..
         "within current screen.", "ws 4"),
    kpress(META.."5", "WScreen.switch_nth(_, 4)"),
    bdoc("Switch to object 5 (workspace, full screen client window) "..
         "within current screen.", "ws 5"),
    kpress(META.."6", "WScreen.switch_nth(_, 5)"),
    bdoc("Switch to object 6 (workspace, full screen client window) "..
         "within current screen.", "ws 6"),
    kpress(META.."7", "WScreen.switch_nth(_, 6)"),
    bdoc("Switch to object 7 (workspace, full screen client window) "..
         "within current screen.", "ws 7"),
    kpress(META.."8", "WScreen.switch_nth(_, 7)"),
    bdoc("Switch to object 8 (workspace, full screen client window) "..
         "within current screen.", "ws 8"),
    kpress(META.."9", "WScreen.switch_nth(_, 8)"),
    bdoc("Switch to object 9 (workspace, full screen client window) "..
         "within current screen.", "ws 9"),
    kpress(META.."0", "WScreen.switch_nth(_, 9)"),

    bdoc("Switch to next object (workspace, full screen client window) "..
         "within current screen.", "->ws"),
    kpress(META.."grave", "WScreen.switch_next(_)"),
    bdoc("Switch to previous object (workspace, full screen client window) "..
         "within current screen.", "<-ws"),
    kpress(ALTMETA.."asciitilde", "WScreen.switch_prev(_)"),

    submap(META.."O", {
        bdoc("Open first region demanding attention or previously active one."),
        kpress("O", "mod_menu.grabmenu(_, _sub, 'focuslist')"),
        -- Alternative without (cyclable) menu
        --kpress("O", "ioncore.goto_activity() or ioncore.goto_previous()"),

        --bdoc("Go to previous active object."),
        --kpress("O", "ioncore.goto_previous()"),

        --bdoc("Go to first object on activity/urgency list."),
        --kpress("I", "ioncore.goto_activity()"),

    }),

    bdoc("Go to screen 0 on multihead setup.", "scr 0"),
    kpress(ALTMETA.."1", "ioncore.goto_nth_screen(0)"),
    kpress(META.."F1", "ioncore.goto_nth_screen(0)"),
    bdoc("Go to screen 1 on multihead setup.", "scr 1"),
    kpress(ALTMETA.."2", "ioncore.goto_nth_screen(1)"),
    kpress(META.."F2", "ioncore.goto_nth_screen(1)"),
    bdoc("Go to screen 2 on multihead setup.", "scr 2"),
    kpress(ALTMETA.."3", "ioncore.goto_nth_screen(2)"),
    kpress(META.."F3", "ioncore.goto_nth_screen(2)"),
    bdoc("Go to screen 3 on multihead setup.", "scr 3"),
    kpress(ALTMETA.."4", "ioncore.goto_nth_screen(3)"),
    -- kpress(META.."F4", "ioncore.goto_nth_screen(3)"),

    bdoc("Go to previous screen on multihead setup.", "<-scr"),
    kpress(ALTMETA.."Escape", "ioncore.goto_prev_screen()"),
    bdoc("Go to next screen on multihead setup.", "->scr"),
    kpress(META.."Escape", "ioncore.goto_next_screen()"),

    bdoc("Create a new workspace of chosen default type."),
    kpress(META.."F9", "ioncore.create_ws(_)"),

    bdoc("Display the main menu."),
    kpress("F12", "mod_menu.menu(_, _sub, 'mainmenu', {big=true})"),
    kpress(META.."F12", "mod_query.query_menu(_, _sub, 'mainmenu', 'Main menu:')"),
    mpress("Button3", "mod_menu.pmenu(_, _sub, 'mainmenu')"),

    bdoc("Display the window list menu."),
    mpress("Button2", "mod_menu.pmenu(_, _sub, 'windowlist')"),

    bdoc("Forward-circulate focus.", "->frame"),
    -- '_chld' used here stands to for an actual child window that may not
    -- be managed by the screen itself, unlike '_sub', that is likely to be
    -- the managing group of that window. The right/left directions are
    -- used instead of next/prev, because they work better in conjunction
    -- with tilings.
    kpress(META.."Tab", "ioncore.goto_next(_chld, 'right')",
           "_chld:non-nil"),

    bdoc("Backward-circulate focus.", "<-frame"),
    kpress(ALTMETA.."Tab", "ioncore.goto_next(_chld, 'left')",
           "_chld:non-nil"),

    bdoc("Raise focused object, if possible.", "raise"),
    kpress(ALTMETA.."R", "WRegion.rqorder(_chld, 'front')",
           "_chld:non-nil"),
})


-- Client window bindings
--
-- These bindings affect client windows directly.

defbindings("WClientWin", {
    bdoc("Nudge the client window. This might help with some "..
      "programs' resizing problems.", "nudge"),
    kpress_wait(ALTMETA.."L", "WClientWin.nudge(_)"),

    bdoc("Kill client owning the client window.", "kill"),
    kpress(ALTMETA.."C", "WClientWin.kill(_)"),

    bdoc("Send next key press to the client window. "..
         "Some programs may not allow this by default.", "quote"),
    kpress(ALTMETA.."Q", "WClientWin.quote_next(_)"),
})


-- Client window group bindings

defbindings("WGroupCW", {
    bdoc("Toggle client window group full-screen mode", "fullscr"),
    kpress_wait(META.."Q", "WGroup.set_fullscreen(_, 'toggle')"),
})


-- WMPlex context bindings
--
-- These bindings work in frames and on screens. The innermost of such
-- contexts/objects always gets to handle the key press.

defbindings("WMPlex", {
    bdoc("Close current object.", "close"),
    kpress_wait(META.."C", "WRegion.rqclose_propagate(_, _sub)"),

    bdoc("Detach (float) or reattach an object to its previous location.", "detach"),
    -- By using _chld instead of _sub, we can detach/reattach queries
    -- attached to a group. The detach code checks if the parameter
    -- (_chld) is a group 'bottom' and detaches the whole group in that
    -- case.
    kpress(ALTMETA.."D", "ioncore.detach(_chld, 'toggle')", "_chld:non-nil"),
})

-- Frames for transient windows ignore this bindmap

defbindings("WMPlex.toplevel", {
    bdoc("Toggle tag of current object.", "tag"),
    kpress(META.."T", "WRegion.set_tagged(_sub, 'toggle')", "_sub:non-nil"),

    bdoc("Clear all tags.", "-tags"),
    kpress(ALTMETA.."T", "ioncore.clear_tags()"),

    bdoc("Lock screen", "lock"),
    kpress(META.."L", "notioncore.exec_on(_, notioncore.lookup_script('notion-lock'))"),

    bdoc("Run a terminal emulator.", "xterm"),
    kpress(META.."Return", "mod_query.exec_on_merr(_, XTERM or 'xterm')"),
    kpress("F2", "mod_query.exec_on_merr(_, XTERM or 'xterm')"),

    bdoc("Query for command line to execute.", "run"),
    kpress(META.."J", "mod_query.query_exec(_)"),

    bdoc("Query for Lua code to execute.", "lua"),
    kpress(ALTMETA.."J", "mod_query.query_lua(_)"),

    bdoc("Query for host to connect to with SSH.", "ssh"),
    kpress(ALTMETA.."F4", "mod_query.query_ssh(_, ':ssh')"),

    bdoc("Query for file to edit.", "edit"),
    kpress(ALTMETA.."F5",
           "mod_query.query_editfile(_, 'run-mailcap --action=edit')"),

    bdoc("Query for file to view.", "view"),
    kpress(ALTMETA.."F6",
           "mod_query.query_runfile(_, 'run-mailcap --action=view')"),

    bdoc("Query for keybinding.", "qkb"),
    kpress(META.."F7",
           "mod_query.query_binding(_, _sub)"),

    bdoc("Query for workspace to go to or create a new one.", "+ws"),
    kpress(ALTMETA.."F9", "mod_query.query_workspace(_)"),

    bdoc("Query for a client window to go to.", "go"),
    kpress(META.."G", "mod_query.query_gotoclient(_)"),

    bdoc("Display context menu.", "ctx"),
    kpress(META.."C", "mod_menu.menu(_, _sub, 'ctxmenu')"),
    bdoc("Query for context menu.", "qctx"),
    kpress(ALTMETA.."C", "mod_query.query_menu(_, _sub, 'ctxmenu', 'Context menu:')"),

    bdoc("Show Notion 'live docs'.", "help"),
    kpress(META.."slash", "notioncore.show_live_docs(_)"),

    bdoc("Query for manual page to be displayed.", "man"),
    kpress(ALTMETA.."slash", "mod_query.query_man(_, ':man')"),
}) 

-- WFrame context bindings
--
-- These bindings are common to all types of frames. Some additional
-- frame bindings are found in some modules' configuration files.

defbindings("WFrame", {
    bdoc("Maximize the frame horizontally.", "hmax"),
    kpress(ALTMETA.."H", "WFrame.maximize_horiz(_)"),
    bdoc("Maximize the frame vertically.", "vmax"),
    kpress(ALTMETA.."V", "WFrame.maximize_vert(_)"),

    bdoc("Display context menu."),
    mpress("Button3", "mod_menu.pmenu(_, _sub, 'ctxmenu')"),

    bdoc("Begin move/resize mode.", "resize"),
    kpress(META.."R", "WFrame.begin_kbresize(_)"),

    bdoc("Switch the frame to display the object indicated by the tab."),
    mclick("Button1@tab", "WFrame.p_switch_tab(_)"),
    mclick("Button2@tab", "WFrame.p_switch_tab(_)"),

    bdoc("Resize the frame."),
    mdrag("Button1@border", "WFrame.p_resize(_)"),
    mdrag(META.."Button3", "WFrame.p_resize(_)"),

    bdoc("Move the frame."),
    mdrag(META.."Button1", "WFrame.p_move(_)"),

    bdoc("Move objects between frames by dragging and dropping the tab."),
    mdrag("Button1@tab", "WFrame.p_tabdrag(_)"),
    mdrag("Button2@tab", "WFrame.p_tabdrag(_)"),

    bdoc("Switch to next object within the frame.", "->tab"),
    -- See docs on how to disable capslock caps behaviour
    kpress(META.."Caps_Lock", "WFrame.switch_next(_)"),
    mclick(META.."Button4", "WFrame.switch_next(_)"),

    bdoc("Switch to previous object within the frame.", "<-tab"),
    kpress(ALTMETA.."Caps_Lock", "WFrame.switch_prev(_)"),
    mclick(META.."Button5", "WFrame.switch_prev(_)"),
})

-- Frames for transient windows ignore this bindmap

defbindings("WFrame.toplevel", {
    bdoc("Query for a client window to attach ('nick').", "nick"),
    kpress(META.."N", "mod_query.query_attachclient(_)"),

    bdoc("Switch to tab 0 in this frame.", "tab 0"),
    kpress(META.."A", "WFrame.switch_nth(_, 0)"),
    bdoc("Switch to tab 1 in this frame.", "tab 1"),
    kpress(META.."S", "WFrame.switch_nth(_, 1)"),
    bdoc("Switch to tab 2 in this frame.", "tab 2"),
    kpress(META.."D", "WFrame.switch_nth(_, 2)"),
    bdoc("Switch to tab 3 in this frame.", "tab 3"),
    kpress(META.."F", "WFrame.switch_nth(_, 3)"),

    bdoc("Move current tab to the right within the frame.", "tab->"),
    kpress(ALTMETA.."comma", "WFrame.dec_index(_, _sub)", "_sub:non-nil"),
    bdoc("Move current tab to the left within the frame.", "tab<-"),
    kpress(ALTMETA.."period", "WFrame.inc_index(_, _sub)", "_sub:non-nil"),

    bdoc("Maximize the frame horizontally/vertically."),
    kpress(ALTMETA.."H", "WFrame.maximize_horiz(_)"),
    kpress(ALTMETA.."V", "WFrame.maximize_vert(_)"),

    bdoc("Attach tagged objects to this frame."),
    kpress(META.."N", "ioncore.tagged_attach(_)"),
})

-- Bindings for floating frames

defbindings("WFrame.floating", {
    bdoc("Toggle shade mode"),
    mdblclick("Button1@tab", "WFrame.set_shaded(_, 'toggle')"),

    bdoc("Raise the frame."),
    mpress("Button1@tab", "WRegion.rqorder(_, 'front')"),
    mpress("Button1@border", "WRegion.rqorder(_, 'front')"),
    mclick(META.."Button1", "WRegion.rqorder(_, 'front')"),

    bdoc("Lower the frame."),
    mclick(META.."Button3", "WRegion.rqorder(_, 'back')"),

    bdoc("Move the frame."),
    mdrag("Button1@tab", "WFrame.p_move(_)"),
})


-- WMoveresMode context bindings
--
-- These bindings are available keyboard move/resize mode. The mode
-- is activated on frames with the command begin_kbresize (bound to
-- META.."R" above by default).

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
    kpress(META.."Left",  "WMoveresMode.move(_,-1, 0)"),
    kpress(META.."Right", "WMoveresMode.move(_, 1, 0)"),
    kpress(META.."Up",    "WMoveresMode.move(_, 0,-1)"),
    kpress(META.."Down",  "WMoveresMode.move(_, 0, 1)"),
    kpress(META.."F",     "WMoveresMode.move(_,-1, 0)"),
    kpress(META.."B",     "WMoveresMode.move(_, 1, 0)"),
    kpress(META.."P",     "WMoveresMode.move(_, 0,-1)"),
    kpress(META.."N",     "WMoveresMode.move(_, 0, 1)"),
})


--
-- Menu definitions
--


-- Main menu
defmenu("mainmenu", {
    menuentry("Run...",         "mod_query.query_exec(_)"),
    menuentry("Terminal",       "mod_query.exec_on_merr(_, XTERM or 'xterm')"),
    menuentry("Lock screen",
        "notioncore.exec_on(_, notioncore.lookup_script('notion-lock'))"),
    menuentry("Help",           "mod_query.query_man(_)"),
    menuentry("About Notion",      "mod_query.show_about_ion(_)"),
    submenu("Styles",           "stylemenu"),
    submenu("Session",          "sessionmenu"),
})


-- Session control menu
defmenu("sessionmenu", {
    menuentry("Save",           "ioncore.snapshot()"),
    menuentry("Restart",        "ioncore.restart()"),
    menuentry("Restart TWM",    "ioncore.restart_other('twm')"),
    menuentry("Exit",           "ioncore.shutdown()"),
})


-- Context menu (frame actions etc.)
defctxmenu("WFrame", "Frame", {
    -- Note: this propagates the close to any subwindows; it does not
    -- destroy the frame itself, unless empty. An entry to destroy tiled
    -- frames is configured in cfg_tiling.lua.
    menuentry("Close",          "WRegion.rqclose_propagate(_, _sub)"),
    -- Low-priority entries
    menuentry("Attach tagged", "ioncore.tagged_attach(_)", { priority = 0 }),
    menuentry("Clear tags",    "ioncore.clear_tags()", { priority = 0 }),
    menuentry("Window info",   "mod_query.show_tree(_, _sub)", { priority = 0 }),
})


-- Context menu for groups (workspaces, client windows)
defctxmenu("WGroup", "Group", {
    menuentry("Toggle tag",     "WRegion.set_tagged(_, 'toggle')"),
    menuentry("De/reattach",    "ioncore.detach(_, 'toggle')"),
})


-- Context menu for workspaces
defctxmenu("WGroupWS", "Workspace", {
    menuentry("Close",          "WRegion.rqclose(_)"),
    menuentry("Rename",         "mod_query.query_renameworkspace(nil, _)"),
    menuentry("Attach tagged",  "ioncore.tagged_attach(_)"),
})


-- Context menu for client windows
defctxmenu("WClientWin", "Client window", {
    menuentry("Kill",           "WClientWin.kill(_)"),
})
