--
-- Ion menu definitions
--


-- Main menu
defmenu("mainmenu", {
    submenu("Programs", 	  "appmenu"),
    menuentry("Lock screen", 	  "ioncore.exec('xlock')"),
    menuentry("Help", 		  "querylib.query_man(_)"),
    menuentry("About Ion", 	  "querylib.show_about_ion(_)"),
    submenu("Styles", 		  "stylemenu"),
    submenu("Exit", 		  "exitmenu"),
})


-- Application menu
defmenu("appmenu", {
    menuentry("XTerm", 		  "ioncore.exec('xterm')"),
    menuentry("Mozilla Firebird", "ioncore.exec('MozillaFirebird')"),
    menuentry("Run...", 	  "querylib.query_exec(_)"),
})


-- Menu with restart/exit alternatives
defmenu("exitmenu", {
    menuentry("Restart", 	  "ioncore.restart()"),
    menuentry("Restart PWM", 	  "ioncore.restart_other('pwm')"),
    menuentry("Restart TWM",      "ioncore.restart_other('twm')"),
    menuentry("Exit", 		  "ioncore.exit()"),
})


-- Context menu (frame/client window actions)
defmenu("ctxmenu", {
    menuentry("Close", 		  "ioncorelib.propagate_close(_, _sub)"),
    menuentry("Kill", 		  "WClientWin.kill(_sub)",
                             	  "_sub:WClientWin"),
    menuentry("(Un)tag", 	  "WRegion.toggle_tag(_sub)",
                                  "_sub:non-nil"),
    menuentry("Attach tagged", 	  "WFrame.attach_tagged(_)"),
    menuentry("Clear tags", 	  "ioncore.clear_tags()"),
})


-- Context menu for floating frames -- add sticky toggle.
defmenu("ctxmenu-floatframe", {
    menuentry("Close", 		  "ioncorelib.propagate_close(_, _sub)"),
    menuentry("Kill", 		  "WClientWin.kill(_sub)",
                             	  "_sub:WClientWin"),
    menuentry("(Un)tag", 	  "WRegion.toggle_tag(_sub)",
                                  "_sub:non-nil"),
    menuentry("Attach tagged", 	  "WFrame.attach_tagged(_)"),
    menuentry("Clear tags", 	  "ioncore.clear_tags()"),
    menuentry("(Un)stick", 	  "WFloatFrame.toggle_sticky(_)"),
})

