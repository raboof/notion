-- look-simpleblue.lua drawing engine configuration file for Ion.

if not gr_select_engine("de") then return end

de_reset()

de_define_style("*", {
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

de_define_style("frame", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    padding_colour = "black",
    background_colour = "black",
    foreground_colour = "#ffffff",
    padding_pixels = 0,
    highlight_pixels = 0,
    shadow_pixels = 0,
    de_substyle("active", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "black",
        foreground_colour = "#ffffff",
    }),
})

de_define_style("frame-ionframe", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 0,
    spacing = 1,
})

de_define_style("frame-floatframe", {
    based_on = "frame",
    border_style = "ridge"
})

de_define_style("tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    de_substyle("active-selected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#3f3f3f",
        foreground_colour = "#9f9f9f",
    }),
    de_substyle("active-unselected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#0f1f4f",
        foreground_colour = "#9f9f9f",
    }),
    de_substyle("inactive-selected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#0f1f4f",
        foreground_colour = "#9f9f9f",
    }),
    de_substyle("inactive-unselected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#0f1f4f",
        foreground_colour = "#9f9f9f",
    }),
    text_align = "center",
})

de_define_style("tab-frame", {
    based_on = "tab",
    de_substyle("*-*-*-*-activity", {
        shadow_colour = "black",
        highlight_colour = "black",
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
})

de_define_style("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-18-*-*-*-*-*-*-*",
    padding_pixels = 10,
})

de_define_style("input", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    background_colour = "#3f3f3f",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 0,
    shadow_pixels = 0,
    border_style = "elevated",
    de_substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "#3f3f3f",
    }),
    de_substyle("*-selection", {
        background_colour = "black",
        foreground_colour = "white",
    }),
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
})

gr_refresh()

