--[[
    look_awesome_yaarg.lua (based on look awesome )

    To completely yaargify Ion, the terminal emulator background is recommended to
    be set to gray25 as does the root window. Transparent frames by default 
    wallpaper can be good sometimes... Also note this theme uses terminus font.

    Setup: Drop into ~/.ion3/ or install in the relevant system-wide directory

    Author: James Gray <j dot gray at ed dot ac dot uk> (yaarg in #ion)

    Date: Fri Feb  3 00:13:43 GMT 2006
]]


if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour     = "gray25",
    highlight_colour  = "gray25",
    padding_colour    = "gray25",
    foreground_colour = "white",
    background_colour = "gray25",

    border_style      = "inlaid",

    highlight_pixels  = 0,
    shadow_pixels     = 0,
    padding_pixels    = 0,
    spacing           = 0,

    font              = "-*-terminus-*-*-normal--12-*-*-*-*-*-*-*",

    text_align        = "center",
})

de.defstyle("frame", {
    based_on               = "*",
    background_colour      = "gray25",
    transparent_background = true,

    shadow_pixels = 1,
    padding_pixels = 0,
    highlight_pixels = 0,
    spacing = 1,

    shadow_colour = "gray30",
    highlight_colour = "gray28",

})

de.defstyle("frame-floatframe", {
    based_on       = "frame",

    padding_pixels = 1,

    de.substyle("active", {
        padding_colour = "gray30",
    }),

    de.substyle("inactive", {
        padding_colour = "#808080",
    }),
})

de.defstyle("tab", {
    based_on = "*",

    highlight_pixels = 1,
    shadow_pixels    = 1,
    padding_pixels   = 0,
    spacing          = 0,

    transparent_background = false,

    text_align = "center",

    de.substyle("active-selected", {
        shadow_colour     = "#808080",
        highlight_colour  = "#808080",
        background_colour = "gray33",
        foreground_colour = "white",
    }),

    de.substyle("active-unselected", {
        shadow_colour     = "#808080",
        highlight_colour  = "#808080",
        background_colour = "gray30",
        foreground_colour = "white",
    }),

    de.substyle("inactive-selected", {
        shadow_colour     = "#808080",
        highlight_colour  = "#808080",
        background_colour = "gray33",
        foreground_colour = "white",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour     = "#808080",
        highlight_colour  = "#808080",
        background_colour = "gray30",
        foreground_colour = "white",
    }),

})

de.defstyle("stdisp", {
    padding = 0,
    shadow_pixels = 0,
    padding_pixels = 0,
    spacing = 1,
    highlight_pixels = 0,
    shadow_colour      = "black",
    highlight_colour   = "black",
    background_colour  = "gray40",
    foreground_colour  = "white",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    font = "-*-terminus-*-*-normal--12-*-*-*-*-*-*-*",
    padding_pixels = 1,
    spacing = 0,

    shadow_colour = "red",
    padding_colour = "red",
    highlight_colour = "red",
    background_colour = "red",

    de.substyle("active-*-*-*-activity", {
        shadow_colour     = "red",
        highlight_colour  = "red",
        background_colour = "#808080",
        foreground_colour = "white",
    }),
    de.substyle("inactive-*-*-*-activity", {
        shadow_colour     = "#808080",
        highlight_colour  = "#808080",
        background_colour = "#808080",
        foreground_colour = "#808080",
    }),
})

de.defstyle("tab-frame-ionframe", {
    based_on = "tab-frame",
})

de.defstyle("tab-frame-floatframe", {
    based_on = "tab-frame",
    padding_pixels = 0,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",

    padding_pixels = 1,
    spacing = 2,

    text_align = "left",
})

de.defstyle("tab-menuentry-bigmenu", {
    based_on = "tab-menuentry",

    padding_pixels = 7,
})
    
de.defstyle("tab-menuentry-pmenu", {
    based_on = "tab-menuentry",
    de.substyle("inactive-selected", {
        shadow_colour = "#808080",
        highlight_colour = "#808080",
        background_colour = "#CCCCCC",
        foreground_colour = "#FFF",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#667",
        highlight_colour = "#667",
        background_colour = "#334",
        foreground_colour = "#999",
    }),
 })

de.defstyle("input", {
    based_on = "*",

    foreground_colour = "white",
    background_colour = "grey30",
    padding_colour = "white",

    transparent_background = false,

    border_style = "elevated",

    padding_pixels = 2,
})

de.defstyle("input-edln", {
    based_on = "input",

    de.substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "black",
    }),
    de.substyle("*-selection", {
        background_colour = "#AAA",
        foreground_colour = "#334",
   }),
})

de.defstyle("input-message", {
    based_on = "input",
})

de.defstyle("input-menu", {
    based_on = "input",

    transparent_background = false,

    highlight_pixels = 0,
    shadow_pixels = 0,
    padding_pixels = 0,
    spacing = 0,
})

de.defstyle("input-menu-bigmenu", {
    based_on = "input-menu",
})

de.defstyle("moveres_display", {
    based_on = "input",
})

de.defstyle("dock", {
    based_on = "*",
})

gr.refresh()

