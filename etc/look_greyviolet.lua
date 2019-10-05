-- look_greyviolet.lua drawing engine configuration file for Notion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "#777777",
    highlight_colour = "#eeeeee",
    background_colour = "#aaaaaa",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    shadow_colour = "#777777",
    highlight_colour = "#eeeeee",
    padding_colour = "#aaaaaa",
    background_colour = "#000000",
    foreground_colour = "#ffffff",
    padding_pixels = 2,
    highlight_pixels = 1,
    shadow_pixels = 1,
})

de.defstyle("tab", {
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "#333366",
        highlight_colour = "#aaaacc",
        background_colour = "#666699",
        foreground_colour = "#eeeeee",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#777777",
        highlight_colour = "#eeeeee",
        background_colour = "#aaaaaa",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#777788",
        highlight_colour = "#eeeeff",
        background_colour = "#9999aa",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#777777",
        highlight_colour = "#eeeeee",
        background_colour = "#aaaaaa",
        foreground_colour = "#000000",
    }),
    text_align = "center",
})

de.defstyle("input", {
    shadow_colour = "#777777",
    highlight_colour = "#eeeeee",
    background_colour = "#aaaaaa",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#aaaaaa",
    }),
    de.substyle("*-selection", {
        background_colour = "#aaaaaa",
        foreground_colour = "black",
    }),
})

dopath("lookcommon_emboss")

gr.refresh()

