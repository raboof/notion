--
-- Query module configuration.
--
-- Only bindings that are in effect in queries and message displays are
-- configured here. Actions to display queries are configured in
-- ion-bindings.lua
-- 


defbindings("WEdln", {
    kpress("Control+F", "forward"),
    kpress("Control+B", "back"),
    kpress("Control+E", "eol"),
    kpress("Control+A", "bol"),
    kpress("Control+Z", "bskip_word"),
    kpress("Control+X", "skip_word"),

    kpress("Control+D", "delete"),
    kpress("Control+H", "backspace"),
    kpress("Control+J", "kill_to_eol"),
    kpress("Control+Y", "kill_line"),
    kpress("Control+W", "kill_word"),
    kpress("Control+O", "bkill_word"),

    kpress("Control+P", "history_prev"),
    kpress("Control+N", "history_next"),

    kpress("Control+M", "finish"),
    
    submap("Control+K", {
        kpress("AnyModifier+B", "set_mark"),
        kpress("AnyModifier+Y", "cut"),
        kpress("AnyModifier+K", "copy"),
        kpress("AnyModifier+C", "paste"),
        kpress("AnyModifier+G", "clear_mark"),
    }),

    kpress("Return", "finish"),
    kpress("KP_Enter", "finish"),
    kpress("Delete", "delete"),
    kpress("BackSpace", "backspace"),
    kpress("Home", "bol"),
    kpress("End", "eol"),
    kpress("Tab", "complete"),
    kpress("Up", "history_prev"),
    kpress("Down", "history_next"),
    kpress("Right", "forward"),
    kpress("Left", "back"),

    mclick("Button2", "paste"),
})

defbindings("WInput", {
    kpress("Escape", "cancel"),
    kpress("Control+G", "cancel"),
    kpress("Control+C", "cancel"),
    kpress("Control+U", "scrollup"),
    kpress("Control+V", "scrolldown"),
    kpress("Page_Up", "scrollup"),
    kpress("Page_Down", "scrolldown"),
})

