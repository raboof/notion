--
-- Menu definitions
--


-- Load a library with some convenience functions.
include("menulib")
-- Load query support code
include("querylib")

-- Define a function to display an about mesage.
local function about(mplex)
    query_message(mplex, ioncore_aboutmsg())
end

-- Application menu
defmenu("appmenu", {
    menuentry("XTerm", make_exec_fn("xterm")),
    menuentry("Mozilla Firebird", make_exec_fn("MozillaFirebird")),
    menuentry("Xdvi", make_exec_fn("xdvi")),
    menuentry("GV", make_exec_fn("gv")),
    -- The query module must also be loaded for this binding to work.
    menuentry("Run...", querylib.query_exec),
})

-- Menu with restart/exit alternatives
defmenu("exitmenu", {
    --menuentry("Restart", querylib.query_restart),
    menuentry("Restart", restart_wm),
    menuentry("Restart PWM", function() restart_other_wm("pwm") end),
    menuentry("Restart TWM", function() restart_other_wm("twm") end),
    --menuentry("Exit", querylib.query_exit),
    menuentry("Exit", exit_wm),
})

-- Main menu (contains the two above as submenus)
defmenu("mainmenu", {
    submenu("Programs", "appmenu"),
    menuentry("Lock screen", make_exec_fn("xlock")),
    menuentry("Help", querylib.query_man),
    menuentry("About Ion", about),
    submenu("Styles", "stylemenu"),
    submenu("Exit", "exitmenu"),
})

-- Context menu (frame/client window actions)
defmenu("ctxmenu", {
    menuentry("Close", WMPlex.close_sub_or_self),
    menuentry("Kill", make_mplex_clientwin_fn(WClientWin.kill)),
    menuentry("(Un)tag", make_mplex_sub_fn(WRegion.toggle_tag)),
    menuentry("Attach tagged", WGenFrame.attach_tagged),
    menuentry("Clear tags", clear_tags),
})
