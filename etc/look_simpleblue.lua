-- look_simpleblue.lua drawing engine configuration file for Ion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "black",
    highlight_colour = "black",
    background_colour = "#0f1f4f",
    foreground_colour = "#9f9f9f",
    padding_pixels = 1,
    highlight_pixels = 0,
    shadow_pixels = 0,
    border_style = "elevated",
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    padding_colour = "black",
    background_colour = "black",
    foreground_colour = "#ffffff",
    padding_pixels = 0,
    highlight_pixels = 0,
    shadow_pixels = 0,
    de.substyle("active", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "black",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("frame-tiled", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 0,
    spacing = 1,
})

de.defstyle("frame-floating", {
    based_on = "frame",
    border_style = "ridge"
})

de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#3f3f3f",
        foreground_colour = "#9f9f9f",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#0f1f4f",
        foreground_colour = "#9f9f9f",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#0f1f4f",
        foreground_colour = "#9f9f9f",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#0f1f4f",
        foreground_colour = "#9f9f9f",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#907070",
        highlight_colour = "#907070",
        background_colour = "#990000",
        foreground_colour = "#eeeeee",
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

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    background_colour = "#3f3f3f",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 0,
    shadow_pixels = 0,
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "#3f3f3f",
    }),
    de.substyle("*-selection", {
        background_colour = "black",
        foreground_colour = "white",
    }),
})

dopath("lookcommon_clean")

gr.refresh()

