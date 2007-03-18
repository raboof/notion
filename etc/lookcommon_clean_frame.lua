-- Common frame settings for the "clean" styles

de.defstyle("frame", {
    based_on = "*",
    background_colour = "#000000",
    transparent_background = false,
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
    spacing = 1,
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
