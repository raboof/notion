-- look_cleanios.lua drawing engine configuration file for Ion.

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
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("tab", {
    font = "-*-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "#f0f066",
        highlight_colour = "#f0f066",
        background_colour = "#f0c000",
        foreground_colour = "#000000",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
        background_colour = "#d8d8d8",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
        background_colour = "#a8a8a8",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
        background_colour = "#d8d8d8",
        foreground_colour = "#000000",
    }),
    text_align = "center",
})

de.defstyle("input-edln", {
    de.substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#d8d8d8",
    }),
    de.substyle("*-selection", {
        background_colour = "#f0c000",
        foreground_colour = "#000000",
    }),
})

dopath("lookcommon_clean")

gr.refresh()

