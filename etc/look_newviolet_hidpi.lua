--
-- look_newviolet for Notion's default drawing engine.
-- Based on look_cleanviolet
--

if not gr.select_engine("de") then
    return
end

de.reset()

de.defstyle("*", {
    highlight_colour = "#e7e7ff",
    shadow_colour = "#e7e7ff",
    background_colour = "#b8b8c8",
    foreground_colour = "#000000",

    shadow_pixels = 1,
    highlight_pixels = 2,
    padding_pixels = 1,
    spacing = 1,
    border_style = "elevated",
    border_sides = "tb",

    font = "xft:Source Code Sans:size=16",
    text_align = "center",
})


de.defstyle("tab", {
    based_on = "*",
    font = "xft:Source Code Sans:size=16",

    de.substyle("active-selected", {
        highlight_colour = "#aaaacc",
        shadow_colour = "#aaaacc",
        background_colour = "#666699",
        foreground_colour = "#eeeeee",
    }),

    de.substyle("inactive-selected", {
        highlight_colour = "#cfcfdf",
        shadow_colour = "#cfcfdf",
        background_colour = "#9999bb",
        foreground_colour = "#000000",
    }),
})


de.defstyle("input", {
    based_on = "*",
    text_align = "left",
    highlight_colour = "#eeeeff",
    shadow_colour = "#eeeeff",

    de.substyle("*-selection", {
        background_colour = "#666699",
        foreground_colour = "#000000",
    }),

    de.substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#b8b8c8",
    }),
})


de.defstyle("input-menu", {
    based_on = "input",
    highlight_pixels = 0,
    shadow_pixels = 0,
    padding_pixels = 0,
})


de.defstyle("frame", {
    based_on = "*",
    background_colour = "#000000",
    transparent_background = false,
    shadow_pixels = 1,
    highlight_pixels = 1,
    padding_pixels = 0,
    border_sides = "all",
    de.substyle("quasiactive", {
        -- Something detached from the frame is active
        padding_colour = "#901010",
    }),
})

de.defstyle("frame-tiled", {
    based_on = "frame",
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 1,
})

de.defstyle("frame-tiled-alt", {
    based_on = "frame-tiled",
    bar = "none",
})

de.defstyle("frame-floating", {
    based_on = "frame",
    bar = "shaped",
    padding_pixels = 0,
})

de.defstyle("frame-transient", {
    based_on = "frame",
    bar = "none",
    padding_pixels = 1,
})


dopath("lookcommon_clean_stdisp")
dopath("lookcommon_clean_tab")

-- Refresh objects' brushes.
gr.refresh()
