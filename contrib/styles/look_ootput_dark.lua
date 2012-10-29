-- Authors: ootput
-- License: Unknown
-- Last Changed: Unknown
--
-- look_ootput_dark.lua drawing engine configuration file for Ion.
---- <ootput @ freenode>

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    background_colour = "black",
    foreground_colour = "#204a87",
    font = "profont11",
    border_style = "elevated",
    highlight_colour = "grey20",
    highlight_pixels = 0,
    padding_pixels = 1,
    shadow_colour = "grey20",
    shadow_pixels = 0,
    spacing = 0,
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    padding_colour = "grey10",
    transparent_background = true,
    de.substyle("active", {
        shadow_colour = "black",
        highlight_colour = "black",
        padding_colour = "#204a87",
    }),
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
})

de.defstyle("tab", {
    based_on = "*",
    de.substyle("active-selected", {
        shadow_colour = "#374b83",
        highlight_colour = "#374b83",
        background_colour = "#204a87",
        foreground_colour = "black",
        padding_colour = "black",
    }),
    de.substyle("active-unselected", {
        foreground_colour = "grey40",
        padding_colour = "#000060",
    }),
    de.substyle("inactive-selected", {
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
        foreground_colour = "#ff8171",
    }),
})

de.defstyle("tab-menuentry", {
    based_on = "*",
    text_align = "left",
    de.substyle("active-selected", {
        shadow_colour = "#374b83",
        highlight_colour = "#374b83",
        background_colour = "#204a87",
        foreground_colour = "black",
    }),
    de.substyle("active-unselected", {
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
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    de.substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "black",
    }),
    de.substyle("*-selection", {
        foreground_colour = "#aaaaaa",
    }),
})

de.defstyle("stdisp", {
    based_on = "tab",
    padding_colour = "black",
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
