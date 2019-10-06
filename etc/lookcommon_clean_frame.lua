-- Common frame settings for the "clean" styles

de.defstyle("frame", {
    background_colour = "#000000",
    transparent_background = false,
    de.substyle("quasiactive", {
        -- Something detached from the frame is active
        padding_colour = "#901010",
    }),
})

de.defstyle("frame-tiled", {
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 1,
    spacing = 1,
})

de.defstyle("frame-tiled-alt", {
    bar = "none",
})

de.defstyle("frame-floating", {
    bar = "shaped",
    padding_pixels = 0,
})
