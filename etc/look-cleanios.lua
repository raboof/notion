-- look-cleanios.lua drawing engine configuration file for Ion.

if not gr_select_engine("de") then return end

de_reset()

de_define_style("*", {
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

de_define_style("frame", {
    based_on = "*",
    padding_colour = "#d8d8d8",
    background_colour = "#000000",
    transparent_background = false,
})


de_define_style("frame-ionframe", {
    based_on = "frame",
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 0,
    spacing = 1,
})

de_define_style("tab", {
    based_on = "*",
    font = "-*-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*",
    de_substyle("active-selected", {
        shadow_colour = "#f0f066",
        highlight_colour = "#f0f066",
        background_colour = "#f0c000",
        foreground_colour = "#000000",
    }),
    de_substyle("active-unselected", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
        background_colour = "#d8d8d8",
        foreground_colour = "#000000",
    }),
    de_substyle("inactive-selected", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
        background_colour = "#a8a8a8",
        foreground_colour = "#000000",
    }),
    de_substyle("inactive-unselected", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
        background_colour = "#d8d8d8",
        foreground_colour = "#000000",
    }),
    text_align = "center",
})

de_define_style("tab-frame", {
    based_on = "tab",
    de_substyle("*-*-*-*-activity", {
        shadow_colour = "#ffffff",
        highlight_colour = "#ffffff",
        background_colour = "#990000",
        foreground_colour = "#eeeeee",
    }),
})

de_define_style("tab-frame-ionframe", {
    based_on = "tab-frame",
    spacing = 1,
})

de_define_style("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    spacing = 1,
})

de_define_style("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-18-*-*-*-*-*-*-*",
    padding_pixels = 10,
})

de_define_style("input-edln", {
    based_on = "*",
    de_substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#d8d8d8",
    }),
    de_substyle("*-selection", {
        background_colour = "#f0c000",
        foreground_colour = "#000000",
    }),
})

gr_refresh()

