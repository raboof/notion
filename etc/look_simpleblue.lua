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

de.defstyle("tab", {
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#3f3f3f",
        foreground_colour = "#bfbfbf",
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
        background_colour = "#1f2f4f",
        foreground_colour = "#bfbfbf",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#0f1f4f",
        foreground_colour = "#9f9f9f",
    }),
    text_align = "center",
})

de.defstyle("input", {
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

de.defstyle("input-menu", {
    padding_pixels=0,
})

dopath("lookcommon_clean")

gr.refresh()

