--
-- look-clean.lua drawing engine configuration file for Ion.
-- Style based on the PWM look-clean.conf by db@mosey.org
--

if not gr_select_engine("de") then
    return
end

-- Clear existing styles from memory.
de_reset()

-- Base style
de_define_style("*", {
    -- Gray background
    highlight_colour = "grey",
    shadow_colour = "grey",
    background_colour = "#545d75",
    foreground_colour = "grey",
    
    shadow_pixels = 1,
    highlight_pixels = 1,
    padding_pixels = 1,
    spacing = 0,
    border_style = "elevated",
    
    font = "fixed",
    text_align = "center",
})


de_define_style("frame", {
    based_on = "*",
    transparent_background = false,
    border_style = "ridge",
})


de_define_style("frame-ionframe", {
    based_on = "frame",
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 0,
    spacing = 1,
    padding_colour = "#aaaaaa",
    background_colour = "#000000",
})


de_define_style("frame-tab", {
    based_on = "*",

    de_substyle("active-selected", {
        highlight_colour = "white",
        shadow_colour = "white",
        background_colour = "#8a999e",
        foreground_colour = "white",
    }),

    de_substyle("*-*-*-*-activity", {
        highlight_colour = "grey",
        shadow_colour = "grey",
        background_colour = "#545d75",
        foreground_colour = "#990000",
    }),
})


de_define_style("frame-tab-ionframe", {
    based_on = "frame-tab",
    spacing = 1,
    bar_inside_frame = true,
})


de_define_style("input", {
    based_on = "*",
    
    foreground_colour = "white",

    spacing = 1,
    
    de_substyle("selection", {
        background_colour = "#8a999e",
        foreground_colour = "white",
    }),

    de_substyle("cursor", {
        background_colour = "white",
        foreground_colour = "#545d75",
    }),
})

-- Refresh objects' brushes
gr_refresh()
