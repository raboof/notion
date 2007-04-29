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
    font = "-misc-fixed-medium-r-*-*-13-*-*-*-*-60-*-*",
    text_align = "center",
})

de.defstyle("tab", {
    font = "-misc-fixed-medium-r-*-*-13-*-*-*-*-60-*-*",
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

de.defstyle("input", {
    foreground_colour = "white",
    de.substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "#545d75",
    }),
    de.substyle("*-selection", {
        background_colour = "#aaaaaa",
        foreground_colour = "black",
    }),
    font = "-misc-fixed-medium-r-*-*-13-*-*-*-*-60-*-*",
})

dopath("lookcommon_clean")

de.defstyle("tab-menuentry-big", {
    padding_pixels = 7,
    font = "-misc-fixed-medium-r-*-*-18-*-*-*-*-*-*-*",
})

gr.refresh()

