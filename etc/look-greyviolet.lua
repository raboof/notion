-- look-greyviolet.lua drawing engine configuration file for Ion.

if not gr_select_engine("de") then return end

de_reset()

de_define_style("*", {
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

de_define_style("frame", {
    based_on = "*",
    shadow_colour = "#777777",
    highlight_colour = "#eeeeee",
    padding_colour = "#aaaaaa",
    background_colour = "#000000",
    foreground_colour = "#ffffff",
    padding_pixels = 2,
    highlight_pixels = 1,
    shadow_pixels = 1,
    de_substyle("active", {
        shadow_colour = "#777777",
        highlight_colour = "#eeeeee",
        background_colour = "#aaaaaa",
        foreground_colour = "#ffffff",
    }),
})

de_define_style("frame-ionframe", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
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
        shadow_colour = "#333366",
        highlight_colour = "#aaaacc",
        background_colour = "#666699",
        foreground_colour = "#eeeeee",
    }),
    de_substyle("active-unselected", {
        shadow_colour = "#777777",
        highlight_colour = "#eeeeee",
        background_colour = "#aaaaaa",
        foreground_colour = "#000000",
    }),
    de_substyle("inactive-selected", {
        shadow_colour = "#777788",
        highlight_colour = "#eeeeff",
        background_colour = "#9999aa",
        foreground_colour = "#000000",
    }),
    de_substyle("inactive-unselected", {
        shadow_colour = "#777777",
        highlight_colour = "#eeeeee",
        background_colour = "#aaaaaa",
        foreground_colour = "#000000",
    }),
    text_align = "center",
})

de_define_style("tab-frame", {
    based_on = "tab",
    de_substyle("*-*-*-*-activity", {
        shadow_colour = "#777777",
        highlight_colour = "#eeeeee",
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
    shadow_colour = "#777777",
    highlight_colour = "#eeeeee",
    background_colour = "#aaaaaa",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    de_substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#aaaaaa",
    }),
    de_substyle("*-selection", {
        background_colour = "#aaaaaa",
        foreground_colour = "black",
    }),
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
})

gr_refresh()

