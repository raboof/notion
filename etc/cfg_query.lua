--
-- Query module configuration.
--
-- Only bindings that are in effect in queries and message displays are
-- configured here. Actions to display queries are configured in
-- ion-bindings.lua
-- 


defbindings("WEdln", {
    kpress("Control+F",         "WEdln.forward(_)"),
    kpress("Control+B",         "WEdln.back(_)"),
    kpress("Control+E",         "WEdln.eol(_)"),
    kpress("Control+A",         "WEdln.bol(_)"),
    kpress("Control+Z",         "WEdln.bskip_word(_)"),
    kpress("Control+X",         "WEdln.skip_word(_)"),

    kpress("Control+D",         "WEdln.delete(_)"),
    kpress("Control+H",         "WEdln.backspace(_)"),
    kpress("Control+J",         "WEdln.kill_to_eol(_)"),
    kpress("Control+Y",         "WEdln.kill_line(_)"),
    kpress("Control+W",         "WEdln.kill_word(_)"),
    kpress("Control+O",         "WEdln.bkill_word(_)"),

    kpress("Control+P",         "WEdln.history_prev(_)"),
    kpress("Control+N",         "WEdln.history_next(_)"),

    kpress("Control+M",         "WEdln.finish(_)"),
    
    submap("Control+K", {
        kpress("AnyModifier+B", "WEdln.set_mark(_)"),
        kpress("AnyModifier+Y", "WEdln.cut(_)"),
        kpress("AnyModifier+K", "WEdln.copy(_)"),
        kpress("AnyModifier+C", "WEdln.paste(_)"),
        kpress("AnyModifier+G", "WEdln.clear_mark(_)"),
    }),

    kpress("Return",            "WEdln.finish(_)"),
    kpress("KP_Enter",          "WEdln.finish(_)"),
    kpress("Delete",            "WEdln.delete(_)"),
    kpress("BackSpace",         "WEdln.backspace(_)"),
    kpress("Home",              "WEdln.bol(_)"),
    kpress("End",               "WEdln.eol(_)"),
    kpress("Tab",               "WEdln.complete(_)"),
    kpress("Up",                "WEdln.history_prev(_)"),
    kpress("Down",              "WEdln.history_next(_)"),
    kpress("Right",             "WEdln.forward(_)"),
    kpress("Left",              "WEdln.back(_)"),

    mclick("Button2",           "WEdln.paste(_)"),
})


defbindings("WInput", {
    kpress("Escape",            "WInput.cancel(_)"),
    kpress("Control+G",         "WInput.cancel(_)"),
    kpress("Control+C",         "WInput.cancel(_)"),
    kpress("Control+U",         "WInput.scrollup(_)"),
    kpress("Control+V",         "WInput.scrolldown(_)"),
    kpress("Page_Up",           "WInput.scrollup(_)"),
    kpress("Page_Down",         "WInput.scrolldown(_)"),
})

