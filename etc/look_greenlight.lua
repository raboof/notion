--
-- look_greenlight for Ion's default drawing engine. 
-- 

if not gr.select_engine("de") then
    return
end

-- Clear existing styles from memory.
de.reset()

-- Base style
de.defstyle("*", {
    highlight_colour = "#666666",
    shadow_colour = "#666666",
    background_colour = "#333333",
    foreground_colour = "#cccccc",
    
    shadow_pixels = 1,
    highlight_pixels = 1,
    padding_pixels = 1,
    spacing = 0,
    border_style = "elevated",
    border_sides = "tb",
    
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})


de.defstyle("frame", {
    background_colour = "#000000",
    transparent_background = false,
})


de.defstyle("tab", {
    font = "-*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*",
    spacing = 1,
    
    de.substyle("active-selected", {
        highlight_colour = "#8ac267",
        shadow_colour = "#8ac267",
        background_colour = "#354743",
    }),

    de.substyle("inactive-selected", {
        highlight_colour = "#6aa247",
        shadow_colour = "#6aa247",
    }),
})


de.defstyle("input", {
    text_align = "left",
    spacing = 1,
    
    de.substyle("*-selection", {
        background_colour = "#354743",
    }),

    de.substyle("*-cursor", {
        background_colour = "#8ac267",
        foreground_colour = "#333333",
    }),
})


dopath("lookcommon_clean")

    
-- Refresh objects' brushes.
gr.refresh()
