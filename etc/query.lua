--
-- Query bindings
--

query_wedln_bindings{
    kpress("Control+F", wedln_forward),
    kpress("Control+B", wedln_back),
    kpress("Control+E", wedln_eol),
    kpress("Control+A", wedln_bol),
    kpress("Control+Z", wedln_bskip_word),
    kpress("Control+X", wedln_skip_word),

    kpress("Control+D", wedln_delete),
    kpress("Control+H", wedln_backspace),
    kpress("Control+J", wedln_kill_to_eol),
    kpress("Control+Y", wedln_kill_line),
    kpress("Control+W", wedln_kill_word),
    kpress("Control+O", wedln_bkill_word),

    kpress("Control+P", wedln_history_prev),
    kpress("Control+N", wedln_history_next),

    kpress("Control+M", wedln_finish),
    
    submap("Control+K") {
        kpress("Control+B", wedln_set_mark),
        kpress("Control+Y", wedln_cut),
        kpress("Control+K", wedln_copy),
        kpress("Control+C", wedln_paste),
    },

    kpress("Return", wedln_finish),
    kpress("KP_Enter", wedln_finish),
    kpress("Delete", wedln_delete),
    kpress("BackSpace", wedln_backspace),
    kpress("Home", wedln_bol),
    kpress("End", wedln_eol),
    kpress("Tab", wedln_complete),
    kpress("Up", wedln_history_prev),
    kpress("Down", wedln_history_next),
    kpress("Right", wedln_forward),
    kpress("Left", wedln_back),

    mclick("Button2", wedln_paste),
}

query_bindings{
    kpress("Escape", input_cancel),
    kpress("Control+C", input_cancel),
    kpress("Control+U", input_scrollup),
    kpress("Control+V", input_scrolldown),
    kpress("Page_Up", input_scrollup),
    kpress("Page_Down", input_scrolldown),
}

