--
-- Look-cleanviolet for Ion's default drawing engine. 
-- Based on look-clean and look-violetgrey.
-- 

if not gr_select_engine("de") then
    return
end

-- Clear existing styles from memory.
de_reset()

-- Base style
de_define_style("*", {
    -- Gray background
    highlight_colour = "#eeeeee",
    shadow_colour = "#eeeeee",
    background_colour = "#aaaaaa",
    foreground_colour = "#000000",
    
    shadow_pixels = 1,
    highlight_pixels = 1,
    padding_pixels = 1,
    spacing = 0,
    border_style = "elevated",
    
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    text_align = "center",
})


de_define_style("frame", {
    based_on = "*",
    padding_colour = "#aaaaaa",
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
    
    de_substyle("active-selected", {
        -- Violet tab
        highlight_colour = "#aaaacc",
        shadow_colour = "#aaaacc",
        background_colour = "#666699",
        foreground_colour = "#eeeeee",
    }),

    de_substyle("inactive-selected", {
        -- Greyish violet tab
        highlight_colour = "#eeeeff",
        shadow_colour = "#eeeeff",
        background_colour = "#9999aa",
        foreground_colour = "#000000",
    }),
})


de_define_style("tab-frame", {
    based_on = "tab",

    de_substyle("*-*-*-*-activity", {
        -- Red tab
        highlight_colour = "#eeeeff",
        shadow_colour = "#eeeeff",
        background_colour = "#990000",
        foreground_colour = "#eeeeee",
    }),
})


de_define_style("tab-frame-ionframe", {
    based_on = "tab-frame",
    spacing = 1,
    bar_inside_frame = true,
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


de_define_style("input", {
    based_on = "*",
    
    -- Bigger font for readability
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "left",
    spacing = 1,
    
    -- Greyish violet background
    highlight_colour = "#eeeeff",
    shadow_colour = "#eeeeff",
    background_colour = "#9999aa",
    foreground_colour = "#000000",
    
    de_substyle("*-selection", {
        background_colour = "#777799",
        foreground_colour = "#000000",
    }),

    de_substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#9999aa",
    }),
})

    
-- Refresh objects' brushes.
gr_refresh()
