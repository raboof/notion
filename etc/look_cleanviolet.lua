--
-- Look_cleanviolet for Ion's default drawing engine. 
-- Based on look-clean and look-violetgrey.
-- 

if not gr.select_engine("de") then
    return
end

-- Clear existing styles from memory.
de.reset()

-- Base style
de.defstyle("*", {
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
    
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})


de.defstyle("frame", {
    based_on = "*",
    padding_colour = "#aaaaaa",
    background_colour = "#000000",
    transparent_background = false,
})


de.defstyle("frame-tiled", {
    based_on = "frame",
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 0,
    spacing = 1,
})


de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    
    de.substyle("active-selected", {
        -- Violet tab
        highlight_colour = "#aaaacc",
        shadow_colour = "#aaaacc",
        background_colour = "#666699",
        foreground_colour = "#eeeeee",
    }),

    de.substyle("inactive-selected", {
        -- Greyish violet tab
        highlight_colour = "#eeeeff",
        shadow_colour = "#eeeeff",
        background_colour = "#9999aa",
        foreground_colour = "#000000",
    }),
})


de.defstyle("tab-frame", {
    based_on = "tab",

    de.substyle("*-*-*-*-activity", {
        -- Red tab
        highlight_colour = "#eeeeff",
        shadow_colour = "#eeeeff",
        background_colour = "#990000",
        foreground_colour = "#eeeeee",
    }),
})


de.defstyle("tab-frame-tiled", {
    based_on = "tab-frame",
    spacing = 1,
    bar_inside_frame = true,
})


de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    spacing = 1,
})


de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})


de.defstyle("input", {
    based_on = "*",
    text_align = "left",
    spacing = 1,
    -- Greyish violet background
    highlight_colour = "#eeeeff",
    shadow_colour = "#eeeeff",
    background_colour = "#9999aa",
    foreground_colour = "#000000",
    
    de.substyle("*-selection", {
        background_colour = "#777799",
        foreground_colour = "#000000",
    }),

    de.substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#9999aa",
    }),
})

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    text_align = "left",
    background_colour = "#000000",
    foreground_colour = "grey",
    font="fixed",
    
    de.substyle("important", {
        foreground_colour = "green",
    }),

    de.substyle("critical", {
        foreground_colour = "red",
    }),
})
    
-- Refresh objects' brushes.
gr.refresh()
