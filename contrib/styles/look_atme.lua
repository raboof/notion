-- look_atme.lua drawing engine configuration file for Ion.
-- 
-- Author: Sadrul Habib Chowdhury (Adil)
-- imadil |at| gmail |dot| com

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "grey",
    highlight_colour = "grey",
    background_colour = "#eeeeff",
    foreground_colour = "#444466",
    padding_pixels = 0,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "fixed",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    padding_colour = "#444466",
    background_colour = "black",
    transparent_background = true,
    de.substyle("active", {
        shadow_colour = "grey",
        highlight_colour = "grey",
    }),
})

de.defstyle("frame-ionframe", {
    ionframe_bar_inside_border = true,
})

de.defstyle("frame-tiled", {
    based_on = "frame",
    padding_pixels = 0,
    highlight_pixels = 0,
    shadow_pixels = 0,
    spacing = 1,
})


de.defstyle("tab", {
    based_on = "*",
    de.substyle("active-selected", {
        shadow_colour = "#eeeeff",
        highlight_colour = "#eeeeff",
        background_colour = "#444466",
        foreground_colour = "#eeeeff",
        transparent_background = false,
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#666688",
        highlight_colour = "#666688",
        background_colour = "#666688",
        foreground_colour = "#eeeeff",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "white",
        highlight_colour = "white",
        background_colour = "#999999",
        foreground_colour = "white",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#a0a0a0",
        highlight_colour = "#a0a0a0",
        background_colour = "#a0a0a0",
        foreground_colour = "white",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#664444",
        highlight_colour = "#664444",
        background_colour = "#ffff00",
        foreground_colour = "#990000",
    }),
})

de.defstyle("tab-frame-tiled", {
    based_on = "tab-frame",
    spacing = 1,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
})

de.defstyle("tab-menuentry-pmenu", {
    text_align = "left",
    de.substyle("*-selected", {
        shadow_colour = "#eeeeff",
        highlight_colour = "#eeeeff",
        background_colour = "#444466",
        foreground_colour = "#eeeeff",
    }),
    de.substyle("*-unselected", {
        shadow_colour = "#666688",
        highlight_colour = "#666688",
        background_colour = "#666688",
        foreground_colour = "#eeeeff",
    }),
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "grey",
    highlight_colour = "grey",
    background_colour = "#444466",
    foreground_colour = "#eeeeff",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "#444466",
    }),
    de.substyle("*-selection", {
        background_colour = "#aaaaaa",
        foreground_colour = "white",
    }),
})

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    background_colour = "#444466",
    foreground_colour = "#eeeeff",
    text_align = "left",

    de.substyle("important", {
        foreground_colour = "green",
    }),

    de.substyle("critical", {
        foreground_colour = "red",
        background_colour = "yellow",
    }),

    transparent_background = false,
})

gr.refresh()

