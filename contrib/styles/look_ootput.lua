-- look-ootput.lua drawing engine configuration file for Ion.
---- <http://metawire.org/~ootput>

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "grey20",
    highlight_colour = "grey20",
    background_colour = "black",
    foreground_colour = "#778bb3",
    padding_pixels = 1,
    highlight_pixels = 0,
    shadow_pixels = 0,
    border_style = "elevated",
    font = "profont11",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    padding_colour = "grey20",
    transparent_background = true,
    background_colour = "black",
    de.substyle("active", {
        shadow_colour = "black",
        highlight_colour = "black",
        padding_colour = "#778bb3",
        background_colour = "black",
    }),
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
    padding_pixels = 1,
    highlight_pixels = 0,
    shadow_pixels = 0,
    spacing = 0,
})

de.defstyle("tab", {
    based_on = "*",
    de.substyle("active-selected", {
        shadow_colour = "#374b83",
        highlight_colour = "#374b83",
        background_colour = "#778bb3",
        foreground_colour = "black",
        padding_colour = "black",
    }),
    de.substyle("active-unselected", {
        foreground_colour = "grey40",
        padding_colour = "#000060",
    }),
    de.substyle("inactive-selected", {
        foreground_colour = "#778bb3",
        padding_colour = "#000060",
    }),
    de.substyle("inactive-unselected", {
        foreground_colour = "grey40",
        padding_colour = "#000060",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        background_colour = "#ff8171",
        foreground_colour = "black",
    }),
})

de.defstyle("tab-menuentry", {
    based_on = "*",
    text_align = "left",
    de.substyle("active-selected", {
        shadow_colour = "#374b83",
        highlight_colour = "#374b83",
        background_colour = "#778bb3",
        foreground_colour = "black",
    }),
    de.substyle("active-unselected", {
        foreground_colour = "#778bb3",
    }),
    de.substyle("inactive-selected", {
        background_colour = "#97abd3",
        foreground_colour = "black",
    }),
    de.substyle("inactive-unselected", {
        foreground_colour = "grey40",
    }),
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    padding_pixels = 5,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    background_colour = "#778bb3",
    foreground_colour = "black",
    de.substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "black",
    }),
    de.substyle("*-selection", {
        background_colour = "#aaaaaa",
        foreground_colour = "black",
    }),
})

de.defstyle("stdisp", {
    based_on = "tab",
    background_colour = "#000000",
    padding_colour = "#000000",
    de.substyle("important", { foreground_colour = "#ffff00", }),
    de.substyle("critical", { foreground_colour = "#ff0000", }),
    de.substyle("gray", { foreground_colour = "#505050", }),
    de.substyle("red", { foreground_colour = "#ff0000", }),
    de.substyle("green", { foreground_colour = "#00ff00", }),
    de.substyle("blue", { foreground_colour = "#0000ff", }),
    de.substyle("cyan", { foreground_colour = "#00ffff", }),
    de.substyle("magenta", { foreground_colour = "#ff00ff", }),
    de.substyle("yellow", { foreground_colour = "#ffff00", }),
})

de.defstyle("dock", {
    outline_style = "all",
})

gr.refresh()

