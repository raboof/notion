-- look-ios.lua drawing engine configuration file for Ion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "#606060",
    highlight_colour = "#ffffff",
    background_colour = "#d8d8d8",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    shadow_colour = "#606060",
    highlight_colour = "#ffffff",
    padding_colour = "#d8d8d8",
    background_colour = "#000000",
    foreground_colour = "#000000",
    padding_pixels = 2,
    highlight_pixels = 1,
    shadow_pixels = 1,
    de.substyle("active", {
        shadow_colour = "#606060",
        highlight_colour = "#ffffff",
        background_colour = "#d8d8d8",
        foreground_colour = "#000000",
    }),
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
    spacing = 0,
})

de.defstyle("frame-floatframe", {
    based_on = "frame",
    border_style = "ridge"
})

de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "#f09000",
        highlight_colour = "#f0f066",
        background_colour = "#f0c000",
        foreground_colour = "#000000",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#606060",
        highlight_colour = "#ffffff",
        background_colour = "#d8d8d8",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#606060",
        highlight_colour = "#efefef",
        background_colour = "#a8a8a8",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#606060",
        highlight_colour = "#ffffff",
        background_colour = "#d8d8d8",
        foreground_colour = "#000000",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#606060",
        highlight_colour = "#ffffff",
        background_colour = "#990000",
        foreground_colour = "#eeeeee",
    }),
})

de.defstyle("tab-frame-ionframe", {
    based_on = "tab-frame",
    spacing = 0,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    highlight_pixels = 0,
    shadow_pixels = 0,
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "#606060",
    highlight_colour = "#ffffff",
    background_colour = "#d8d8d8",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
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

