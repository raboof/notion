-- look-awesome.lua drawing engine configuration file for Ion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "#3C5268",
    highlight_colour = "#9DAABA",
    background_colour = "#3C5268",
    foreground_colour = "#FFFFFF",
    padding_colour = "#000000",--#778BA0",

    transparent_background = false,
    
    border_style = "elevated",

    highlight_pixels = 0,
    shadow_pixels = 0,
    padding_pixels = 0,
    spacing = 0,
    
    font = "-xos4-terminus-medium-r-normal--14-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",

-- this sets the color between tabs as well, I could not figure out any way to make it transparent
    background_colour = "#000000",--#778BA0",

    transparent_background = true,
    de.substyle("active", {
     shadow_colour = "#3C5268",
     highlight_colour = "#9DAABA",
     padding_colour = "#000000",--#778BA0",
     background_colour = "#3C5268",
    }),
--    de.substyle("inactive", {
--    }),
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
--    de.substyle("active", {
--    }),
--    de.substyle("inactive", {
--    }),
})

de.defstyle("frame-floatframe", {
    based_on = "frame",
    padding_pixels = 1,
    de.substyle("active", {
        padding_colour = "#99A",
    }),
    de.substyle("inactive", {
        padding_colour = "#666",
    }),
})

de.defstyle("tab", {
    based_on = "*",

    highlight_pixels = 1,
    shadow_pixels = 1,
    padding_pixels = 0,
    spacing = 1,

    transparent_background = true,

    text_align = "center",

    de.substyle("active-selected", {
        shadow_colour = "#3C5268",
        highlight_colour = "#3C5268",
        background_colour = "#3C5268",
        foreground_colour = "#FFFFFF",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#A7B5C6",
        highlight_colour = "#A7B5C6",
        background_colour = "#9DAABA",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#3C5268",
        highlight_colour = "#A7B5C6",
        background_colour = "#9DAABA",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#3C5268",--#939FAE",
        highlight_colour = "#A7B5C6",
        background_colour = "#9DAABA",
        foreground_colour = "#4C4C4C",
    }),
})

de.defstyle("tab-frame", {
    based_on = "tab",
    padding_pixels = 3,
--    de.substyle("*-*-tagged", {
--    }),
--    de.substyle("*-*-*-dragged", {
--    }),
    de.substyle("active-*-*-*-activity", {
        shadow_colour = "red",
        highlight_colour = "red",
    	background_colour = "#800",
        foreground_colour = "#FFF",
    }),
    de.substyle("inactive-*-*-*-activity", {
        shadow_colour = "#800",
        highlight_colour = "#800",
    	background_colour = "#400",
        foreground_colour = "#888",
    }),
})

de.defstyle("tab-frame-ionframe", {
    based_on = "tab-frame",
})

de.defstyle("tab-frame-floatframe", {
    based_on = "tab-frame",
    padding_pixels = 4,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",

    padding_pixels = 6,
    spacing = 0,

    font = "-xos4-terminus-medium-r-normal--16-*-*-*-*-*-*-*",
    text_align = "left",

--    de.substyle("*-*-submenu", {
--    }),
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",

    padding_pixels = 8,

    font = "-xos4-terminus-medium-r-normal--28-*-*-*-*-*-*-*",
})

de.defstyle("tab-menuentry-pmenu", {
    based_on = "tab-menuentry",
    de.substyle("inactive-selected", {
        shadow_colour = "#3C5268",
        highlight_colour = "#3C5268",
        background_colour = "#3C5268",
        foreground_colour = "#FFFFFF",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#A7B5C6",
        highlight_colour = "#A7B5C6",
        background_colour = "#9DAABA",
        foreground_colour = "#000000",
    }),
})

de.defstyle("input", {
    based_on = "*",
    
    foreground_colour = "#000000",
    background_colour = "#9DAABA",
    padding_colour = "#9CAAB4",

    transparent_background = false,

    border_style = "elevated",

    padding_pixels = 3,
})

de.defstyle("input-edln", {
    based_on = "input",

})

de.defstyle("input-message", {
    based_on = "input",
})

de.defstyle("input-menu", {
    based_on = "input",

    de.substyle("*-selection", {
        background_color = "#AAA",
	foreground_color = "#334",
    }),

    de.substyle("*-cursor", {
        background_color = "#FFF",
	foreground_color = "#667",
    }),

    transparent_background = false,

    background_color = "#AAA",
    foreground_color = "#334",
    highlight_pixels = 0,
    shadow_pixels = 0,
    padding_pixels = 0,
    spacing = 0,
})

de.defstyle("input-menu-big", {
    based_on = "input-menu",
})

de.defstyle("moveres_display", {
    based_on = "input-menu",
})

de.defstyle("dock", {
    based_on = "*",
})

gr.refresh()

