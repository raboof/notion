-- look-clean.lua drawing engine configuration file for Ion.

if not gr_select_engine("de") then return end

de_reset()

de_define_style("*", {
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

de_define_style("frame", {
    based_on = "*",
    padding_colour = "#545d75",
    background_colour = "black",
    de_substyle("active", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        padding_colour = "#545d75",
        background_colour = "black",
    }),
})

de_define_style("frame-ionframe", {
    based_on = "frame",
    padding_pixels = 0,
    highlight_pixels = 0,
    shadow_pixels = 0,
    spacing = 1,
})


de_define_style("tab", {
    based_on = "*",
    font = "fixed",
    de_substyle("active-selected", {
        shadow_colour = "white",
        highlight_colour = "white",
        background_colour = "#8a999e",
        foreground_colour = "white",
    }),
    de_substyle("active-unselected", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#545d75",
        foreground_colour = "grey",
    }),
    de_substyle("inactive-selected", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#545d75",
        foreground_colour = "grey",
    }),
    de_substyle("inactive-unselected", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#545d75",
        foreground_colour = "grey",
    }),
    text_align = "center",
})

de_define_style("tab-frame", {
    based_on = "tab",
    de_substyle("*-*-*-*-activity", {
        shadow_colour = "grey",
        highlight_colour = "grey",
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
    de_substyle("inactive-selected", {
        shadow_colour = "grey",
        highlight_colour = "grey",
        background_colour = "#8a999e",
        foreground_colour = "grey",
    }),
})

de_define_style("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-18-*-*-*-*-*-*-*",
    padding_pixels = 10,
})

de_define_style("input", {
    based_on = "*",
    shadow_colour = "grey",
    highlight_colour = "grey",
    background_colour = "#545d75",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    de_substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "#545d75",
    }),
    de_substyle("*-selection", {
        background_colour = "#aaaaaa",
        foreground_colour = "black",
    }),
    font = "fixed",
})

gr_refresh()

