--
-- Query module configuration.
--
-- Only bindings that are in effect in queries and message displays are
-- configured here. Actions to display queries are configured in
-- ion-bindings.lua
-- 


defbindings("WEdln", {
    bdoc("Move one character forward/backward."),
    kpress("Control+F", "WEdln.forward(_)"),
    kpress("Control+B", "WEdln.back(_)"),
    kpress("Right", "WEdln.forward(_)"),
    kpress("Left", "WEdln.back(_)"),
    
    bdoc("Go to end/beginning."),
    kpress("Control+E", "WEdln.eol(_)"),
    kpress("Control+A", "WEdln.bol(_)"),
    kpress("End", "WEdln.eol(_)"),
    kpress("Home", "WEdln.bol(_)"),
    
    bdoc("Skip one word forward/backward."),
    kpress("Control+X", "WEdln.skip_word(_)"),
    kpress("Control+Z", "WEdln.bskip_word(_)"),

    bdoc("Delete next character."),
    kpress("Control+D", "WEdln.delete(_)"),
    kpress("Delete", "WEdln.delete(_)"),
    
    bdoc("Delete previous character."),
    kpress("BackSpace", "WEdln.backspace(_)"),
    kpress("Control+H", "WEdln.backspace(_)"),
    
    bdoc("Delete one word forward/backward."),
    kpress("Control+W", "WEdln.kill_word(_)"),
    kpress("Control+O", "WEdln.bkill_word(_)"),

    bdoc("Delete to end of line."),
    kpress("Control+J", "WEdln.kill_to_eol(_)"),
    
    bdoc("Delete the whole line."),
    kpress("Control+Y", "WEdln.kill_line(_)"),
    
    bdoc("Transpose characters."),
    kpress("Control+T", "WEdln.transpose_chars(_)"),

    bdoc("Select next/previous (matching) history entry."),
    kpress("Control+P", "WEdln.history_prev(_)"),
    kpress("Control+N", "WEdln.history_next(_)"),
    kpress("Up", "WEdln.history_prev(_)"),
    kpress("Down", "WEdln.history_next(_)"),
    kpress("Control+Up", "WEdln.history_prev(_, true)"),
    kpress("Control+Down", "WEdln.history_next(_, true)"),

    bdoc("Paste from the clipboard."),
    mclick("Button2", "WEdln.paste(_)"),
    submap("Control+K", {
        kpress("C", "WEdln.paste(_)"),
        
        bdoc("Set mark/begin selection."),
        kpress("B", "WEdln.set_mark(_)"),
        
        bdoc("Cut selection."),
        kpress("Y", "WEdln.cut(_)"),
        
        bdoc("Copy selection."),
        kpress("K", "WEdln.copy(_)"),
        
        bdoc("Clear mark/cancel selection."),
        kpress("G", "WEdln.clear_mark(_)"),

        --bdoc("Transpose words."),
        --kpress("T", "WEdln.transpose_words(_)"),
    }),

    bdoc("Try to complete the entered text or cycle through completions."),
    kpress("Tab", "WEdln.complete(_, 'next', 'normal')"), 
    kpress("Shift+Tab", "WEdln.complete(_, 'prev', 'normal')"),
    -- Do not cycle; only force evaluation of new completions
    kpress("Control+Tab", "WEdln.complete(_, nil, 'normal')"),
    
    bdoc("Complete from history"),
    kpress("Control+R", "WEdln.complete(_, 'next', 'history')"),
    kpress("Control+S", "WEdln.complete(_, 'prev', 'history')"),
    
    bdoc("Close the query and execute bound action."),
    kpress("Control+M", "WEdln.finish(_)"),
    kpress("Return", "WEdln.finish(_)"),
    kpress("KP_Enter", "WEdln.finish(_)"),
})


defbindings("WInput", {
    bdoc("Close the query/message box, not executing bound actions."),
    kpress("Escape", "WInput.cancel(_)"),
    kpress("Control+G", "WInput.cancel(_)"),
    kpress("Control+C", "WInput.cancel(_)"),
    
    bdoc("Scroll the message or completions up/down."),
    kpress("Control+U", "WInput.scrollup(_)"),
    kpress("Control+V", "WInput.scrolldown(_)"),
    kpress("Page_Up", "WInput.scrollup(_)"),
    kpress("Page_Down", "WInput.scrolldown(_)"),
})


-- Some settings
--[[
mod_query.set{
     -- Auto-show completions?    
     autoshowcompl=true,
     
     -- Delay for completion after latest keypress/modification in 
     -- milliseconds
     autoshowcompl_delay=250,
     
     -- Case-insensitive completion? (Some queries only.)
     caseicompl=true,
     
     -- Sub-string completion? (Some queries only.)
     substrcompl=true,
}
--]]
