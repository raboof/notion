-- look-wheat.lua drawing engine configuration file for Ion.

if not gr_select_engine("de") then return end

de_reset()

de_define_style("*", {
    shadow_colour = "black",
    highlight_colour = "black",
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
    shadow_colour = "black",
    highlight_colour = "black",
    padding_colour = "#7f7f75",
    background_colour = "#353535",
    foreground_colour = "white",
    padding_pixels = 2,
    highlight_pixels = 1,
    shadow_pixels = 2,
    de_substyle("active", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#7f7f75",
        foreground_colour = "white",
    }),
})

de_define_style("frame-ionframe", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
    spacing = 1,
    bar_inside_frame = true,
})

de_define_style("frame-floatframe", {
    based_on = "frame",
    border_style = "ridge"
})

de_define_style("frame-tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    de_substyle("active-selected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#aaaa9e",
        foreground_colour = "white",
    }),
    de_substyle("active-unselected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#7f7f75",
        foreground_colour = "white",
    }),
    de_substyle("inactive-selected", {
        shadow_colour = "black",
        highlight_colour = "black",
        background_colour = "#7f7f75",
        foreground_colour = "white",
    }),
    text_align = "center",
})

de_define_style("frame-tab-ionframe", {
    based_on = "frame-tab",
    spacing = 1,
})

de_define_style("input", {
    based_on = "*",
    shadow_colour = "black",
    highlight_colour = "black",
    background_colour = "#454545",
    foreground_colour = "white",
    padding_pixels = 2,
    highlight_pixels = 1,
    shadow_pixels = 2,
    border_style = "elevated",
    de_substyle("cursor", {
        background_colour = "white",
        foreground_colour = "#454545",
    }),
    de_substyle("selection", {
        background_colour = "black",
        foreground_colour = "white",
    }),
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
})

gr_refresh()
