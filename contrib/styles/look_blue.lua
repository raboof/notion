--
-- look_blue, based on look-cleanviolet
-- 

if not gr.select_engine("de") then
    return
end

-- Clear existing styles from memory.
de.reset()

-- Base style
de.defstyle("*", {
    highlight_colour = "#eeeeff",
    shadow_colour = "#eeeeff",
    background_colour = "#9999bb",
    foreground_colour = "#444477",
    
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
        highlight_colour = "#9999bb",
        shadow_colour = "#9999bb",
        background_colour = "#34639f",
        foreground_colour = "#eeeeff",
    }),

    de.substyle("inactive-selected", {
        highlight_colour = "#dddddd",
        shadow_colour = "#dddddd",
        background_colour = "#7c95b4",
        foreground_colour = "#333366",
    }),
})


de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    spacing = 1,
})


de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 4,
})


de.defstyle("input", {
    based_on = "*",
    text_align = "left",
    spacing = 0,
    highlight_colour = "#9999bb",
    shadow_colour = "#9999bb",
    background_colour = "#34639f",
    foreground_colour = "#eeeeff",
    
    de.substyle("*-selection", {
        background_colour = "#9999ff",
        foreground_colour = "#333366",
    }),

    de.substyle("*-cursor", {
        background_colour = "#ccccff",
        foreground_colour = "#9999aa",
    }),
})

dopath("lookcommon_clean")
    
-- Refresh objects' brushes.
gr.refresh()
