-- look_wheat2.lua drawing engine configuration file for Ion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "gray10",
    highlight_colour = "gray10",
    background_colour = "#7f7f75",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    shadow_colour = "gray10",
    highlight_colour = "gray10",
    padding_colour = "#7f7f75",
    background_colour = "black",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    de.substyle("active", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#7f7f75",
        foreground_colour = "white",
    }),
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
    spacing = 1,
})

de.defstyle("frame-floatframe", {
    based_on = "frame",
    border_style = "ridge"
})

de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#aaaa9e",
        foreground_colour = "white",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#7f7f75",
        foreground_colour = "white",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#7f7f75",
        foreground_colour = "white",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#7f7f75",
        foreground_colour = "gray",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "black",
        highlight_colour = "black",
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
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    background_colour = "#454545",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "#454545",
    }),
    de.substyle("*-selection", {
        background_colour = "black",
        foreground_colour = "white",
    }),
})

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    text_align = "left",
})

gr.refresh()

