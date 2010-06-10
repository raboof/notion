-- look-cleanpastel.lua drawing engine configuration file for Ion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "#ffffff",
    highlight_colour = "#ffffff",
    background_colour = "#d8d8d8",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    spacing = 0,
    border_style = "elevated",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    padding_colour = "#d8d8d8",
    background_colour = "#000000",
    transparent_background = false,
})


de.defstyle("frame-ionframe", {
    based_on = "frame",
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 0,
    spacing = 1,
})

de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "#e0f0e0",
        highlight_colour = "#b0c0b6",
        background_colour = "#70b0a6",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
        background_colour = "#d8d8d8",
        foreground_colour = "#606060",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
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
    spacing = 1,
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-18-*-*-*-*-*-*-*",
    padding_pixels = 10,
})

de.defstyle("input-edln", {
    based_on = "*",
    de.substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#d8d8d8",
    }),
    de.substyle("*-selection", {
        background_colour = "#f0c000",
        foreground_colour = "#000000",
    }),
})

gr.refresh()

