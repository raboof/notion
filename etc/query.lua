--
-- Query module configuration.
--
-- Only bindings that are in effect in queries and message displays are
-- configured here. Actions to display queries are configured in
-- ion-bindings.lua
-- 


query_wedln_bindings{
    kpress("Control+F", WEdln.forward),
    kpress("Control+B", WEdln.back),
    kpress("Control+E", WEdln.eol),
    kpress("Control+A", WEdln.bol),
    kpress("Control+Z", WEdln.bskip_word),
    kpress("Control+X", WEdln.skip_word),

    kpress("Control+D", WEdln.delete),
    kpress("Control+H", WEdln.backspace),
    kpress("Control+J", WEdln.kill_to_eol),
    kpress("Control+Y", WEdln.kill_line),
    kpress("Control+W", WEdln.kill_word),
    kpress("Control+O", WEdln.bkill_word),

    kpress("Control+P", WEdln.history_prev),
    kpress("Control+N", WEdln.history_next),

    kpress("Control+M", WEdln.finish),
    
    submap("Control+K", {
        kpress("AnyModifier+B", WEdln.set_mark),
        kpress("AnyModifier+Y", WEdln.cut),
        kpress("AnyModifier+K", WEdln.copy),
        kpress("AnyModifier+C", WEdln.paste),
        kpress("AnyModifier+G", WEdln.clear_mark),
    }),

    kpress("Return", WEdln.finish),
    kpress("KP_Enter", WEdln.finish),
    kpress("Delete", WEdln.delete),
    kpress("BackSpace", WEdln.backspace),
    kpress("Home", WEdln.bol),
    kpress("End", WEdln.eol),
    kpress("Tab", WEdln.complete),
    kpress("Up", WEdln.history_prev),
    kpress("Down", WEdln.history_next),
    kpress("Right", WEdln.forward),
    kpress("Left", WEdln.back),

    mclick("Button2", WEdln.paste),
}

query_bindings{
    kpress("Escape", WInput.cancel),
    kpress("Control+G", WInput.cancel),
    kpress("Control+C", WInput.cancel),
    kpress("Control+U", WInput.scrollup),
    kpress("Control+V", WInput.scrolldown),
    kpress("Page_Up", WInput.scrollup),
    kpress("Page_Down", WInput.scrolldown),
}


