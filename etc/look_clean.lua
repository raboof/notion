-- look_clean.lua drawing engine configuration file for Ion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "grey",
    highlight_colour = "grey",
    background_colour = "#545d75",
    foreground_colour = "grey",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "fixed",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    padding_colour = "#545d75",
    background_colour = "black",
    de.substyle("active", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        padding_colour = "#545d75",
        background_colour = "black",
    }),
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
    padding_pixels = 0,
    highlight_pixels = 0,
    shadow_pixels = 0,
    spacing = 1,
})


de.defstyle("tab", {
    based_on = "*",
    font = "fixed",
    de.substyle("active-selected", {
        shadow_colour = "white",
        highlight_colour = "white",
        background_colour = "#8a999e",
        foreground_colour = "white",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#545d75",
        foreground_colour = "grey",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#545d75",
        foreground_colour = "grey",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#545d75",
        foreground_colour = "grey",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#990000",
        foreground_colour = "#eeeeee",
    }),
})

de.defstyle("tab-frame-ionframe", {
    based_on = "tab-frame",
    spacing = 1,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    de.substyle("inactive-selected", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#8a999e",
        foreground_colour = "grey",
    }),
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
--    font = "-*-fixed-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "grey",
    highlight_colour = "grey",
    background_colour = "#545d75",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "#545d75",
    }),
    de.substyle("*-selection", {
        background_colour = "#aaaaaa",
        foreground_colour = "black",
    }),
    font = "fixed",
})

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    background_colour = "#000000",
    foreground_colour = "grey",
    text_align = "left",
})

gr.refresh()

