-- look-wheat2.lua drawing engine configuration file for Ion.

if not gr_select_engine("de") then return end

de_reset()

de_define_style("*", {
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

de_define_style("frame", {
    based_on = "*",
    shadow_colour = "gray10",
    highlight_colour = "gray10",
    padding_colour = "#7f7f75",
    background_colour = "black",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    de_substyle("active", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#7f7f75",
        foreground_colour = "white",
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
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#aaaa9e",
        foreground_colour = "white",
    }),
    de_substyle("active-unselected", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#7f7f75",
        foreground_colour = "white",
    }),
    de_substyle("inactive-selected", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#7f7f75",
        foreground_colour = "white",
    }),
    de_substyle("inactive-unselected", {
        shadow_colour = "gray10",
        highlight_colour = "gray10",
        background_colour = "#7f7f75",
        foreground_colour = "#aaaa9e",
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
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})

de_define_style("input", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    background_colour = "#454545",
    foreground_colour = "white",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    de_substyle("*-cursor", {
        background_colour = "white",
        foreground_colour = "#454545",
    }),
    de_substyle("*-selection", {
        background_colour = "black",
        foreground_colour = "white",
    }),
})

gr_refresh()

