-- cfg_mouse.lua
--
-- (Canaan Hadley-Voth)
--
-- A set of bindings intended to make the mouse pull its weight.
-- Fluxbox users should feel right at home.  This will be less 
-- useful without a mouse wheel, but still worth considering.
--
-- Partly it's an experiment to see how far one can get
-- without using a keyboard in a window manager geared
-- toward keyboard users.  But there's nothing inherently
-- anti-mouse about Ion, and with minor adjustments I find it
-- extremely capable.
--
-- To use, add to cfg_ion.lua: dopath("cfg_mouse")

defbindings("WScreen", {

    -- Switching workspaces can be done from anywhere using Mod1 and
    -- the wheel, or without the keyboard by using the wheel over
    -- the left, right or bottom edges of the screen, or over a border
    -- or an empty frame.
    --
    -- (Although a couple of the looks don't have any space at edges
    --  of the screen, most of them do.)
    mpress("Button4", "WScreen.switch_prev(_)"),
    mpress("Button5", "WScreen.switch_next(_)"),
    mpress(MOD1.."Button4", "WScreen.switch_prev(_)"),
    mpress(MOD1.."Button5", "WScreen.switch_next(_)"),

    -- When a WClientWin is set to fullscreen mode, the mouse
    -- can do little about it unless there happen to be gaps
    -- where the "screen" area shines through.  Since this is
    -- often the case esp. with fullscreen terminal emulators, I've
    -- included button2 to exit fullscreen mode, and button3
    -- brings up the main menu by default in cfg_bindings.
    mpress("Button2", "WClientWin.set_fullscreen(_sub, 'toggle')"),
})

defbindings("WFrame", {

    bdoc("Bring up a context menu from anywhere in the frame."),
    mpress(MOD1.."Shift+Button3", "mod_menu.pmenu(_, _sub, 'ctxmenu')"),

    bdoc("Toggle tagged."),
    mpress(MOD1.."Button2", "_sub:set_tagged('toggle')"),
    bdoc("Attach tagged regions."),
    mpress(MOD1.."Shift+Button2", "_:attach_tagged()"),
    
    -- For mouse users I highly recommend button2 as a mainmenu, 
    -- otherwise it would be difficult to start programs using the mouse.
    -- In Ion it's relevant which frame has focus when you start
    -- something, so a file manager would work poorly for that purpose
    -- unless it happened to be in the right frame.
    mpress("Button1@client", "mod_menu.pmenu(_, _sub, 'mainmenu')"),
    mpress("Button2@border", "mod_menu.pmenu(_, _sub, 'mainmenu')"),
    
    bdoc("Maximize vertical on double click"),
    mdblclick("Button1", "WFrame.maximize_vert(_)"),

    -- Use the wheel over the tab bar to scroll through windows in a frame.
    -- Oddly enough, it still works if you've hidden the tab bar.
    -- Just use the wheel over the top edge of the screen.
    -- Or use Mod4 and the wheel to avoid having to aim at anything.
    mpress("Button5@tab", "WScreen.switch_next(_)"),
    mpress("Button4@tab", "WScreen.switch_prev(_)"),

    
    mpress("Button5", "_:parent():switch_next()"),
    mpress("Button4", "_:parent():switch_prev()"),

    mpress(MOD1.."Button4", "_:parent():switch_prev()"),
    mpress(MOD1.."Button5", "_:parent():switch_next()"),
    
    bdoc("Drag without aiming for a tab, using Mod1+Shift"),
    mdrag(MOD1.."Shift+Button1", "WFrame.p_tabdrag(_)"),

--[[ These should already have happened in cfg_bindings.lua.
--  Uncomment if necessary.

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

    mpress("Button3", "mod_menu.pmenu(_, _sub, 'ctxmenu')"),
--]]

    -- Using alsa-utils, by any chance?  Have some volume control.
    --[[
    mpress(MOD1.."Control+Button4", "ioncore.exec_on(_, 'amixer set PCM 1+')"),
    mpress(MOD1.."Control+Button5", "ioncore.exec_on(_, 'amixer set PCM 1-')"),
    --]]
})

if MOD4 then
defbindings("WFrame", {
    -- There were only a few bindings involving MOD4, so
    -- I've fixed this to work with or without.
    -- On my system it's the windows key, defined in 
    -- ~/.ion3/cfg_ion.lua as MOD4="Mod4+"
    bdoc("Show/hide tab"),
    mpress(MOD4.."Button1", "WFrame.set_tabbar(_, 'toggle')"),

    bdoc("Quickly rename a frame."),
    mpress(MOD4.."Shift+Button3", "mod_query.query_renameframe(_)"),

    mpress(MOD4.."Button5", "WScreen.switch_next(_)"),
    mpress(MOD4.."Button4", "WScreen.switch_prev(_)"),
})
else
defbindings("WFrame", {
    bdoc("Show/hide tab"),
    mclick(MOD1.."Shift+Button1", "WFrame.set_tabbar(_, 'toggle')"),

    bdoc("Quickly rename a frame."),
    mpress(MOD1.."Shift+Control+Button3", "mod_query.query_renameframe(_)"),

    mpress(MOD1.."Shift+Button5", "WScreen.switch_next(_)"),
    mpress(MOD1.."Shift+Button4", "WScreen.switch_prev(_)"),
})
end    

if mod_ionws then
defbindings("WFrame-on-WIonWS", {
    mpress("Button2@tab", "mod_menu.pmenu(_, _sub, 'mainmenu')"),
})
end

if mod_sp then
defbindings("WScratchpad", {
    -- button1 to move the scratchpad
    -- button2 to drag a tab out of the scratchpad onto another frame
    -- button2 (full click) to hide the scratchpad
    -- button3 is still a frame context menu
    --
    -- What's missing?  mainmenu via mouse, as described above.
    -- 
    -- Why is that not important?  Because you can button2 on any tab
    -- or border in the background to bring up the main menu, and any
    -- applications you start will land in the scratchpad.  They can't
    -- go anywhere else.  
    mdrag("Button1@tab", "_:p_move()"),
    mdrag("Button2@tab", "_:p_tabdrag()"),
    mclick("Button2", "mod_sp.set_shown_on(_:screen_of(), 'toggle')"),
})
end

if mod_query then
defbindings("WInput", {
    bdoc("No sense reaching for the keyboard just to bat away a WMessage"),
    mpress("Button3", "WInput.cancel(_)"),
})
end

if mod_menu then
defbindings("WMenu", {
    bdoc("...or to close a menu."),
    mpress("Button3", "WMenu.cancel(_)"),
})
end



-- Some additional items should go in the defctxmenu for WFrame
-- (or some other menu called by pmenu) in order to effectively
-- avoid the keyboard.  Paste into cfg_menus.lua if desired.
--
--[[
    menuentry("Toggle Tab", "WFrame.set_tabbar(_, 'toggle')"),
    menuentry("Fullscreen", "WClientWin.set_fullscreen(_sub, 'toggle')", 
                "_sub:WClientWin"),
    menuentry("Show Dock", "mod_dock.set_floating_shown_on(_:screen_of(), 'toggle')"),
    menuentry("Show SP", "mod_sp.set_shown_on(_:screen_of(), 'toggle')"),
 --]]


-- This is a gratuitous alternative way of displaying the scratchpad
-- with a mouse click.  The dockapp wmnet takes an argument for a command 
-- to be executed on a button1 click.  The command reaches into the running
-- Ion3 and toggles the scratchpad.
--
-- Depends on mod_ionflux and (obviously) wmnet.  Also presumes 1st screen.
--[[

wmnet --withdrawn --execute "ionflux -e \"mod_sp.set_shown_on(ioncore.region_list('WScreen')[1], 'toggle')\"" &

--]]
