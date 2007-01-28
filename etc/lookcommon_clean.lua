-- Settings common to some styles.

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    text_align = "left",
    background_colour = "#000000",
    foreground_colour = "grey",
    font="-misc-fixed-medium-r-*-*-13-*-*-*-*-60-*-*",
    
    de.substyle("important", {
        foreground_colour = "green",
    }),

    de.substyle("critical", {
        foreground_colour = "red",
    }),
})

de.defstyle("actnotify", {
    based_on = "*",
    shadow_colour = "#c04040",
    highlight_colour = "#c04040",
    background_colour = "#901010",
    foreground_colour = "#eeeeee",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#c04040",
        highlight_colour = "#c04040",
        background_colour = "#901010",
        foreground_colour = "#eeeeee",
    }),
})

de.defstyle("tab-frame-tiled", {
    based_on = "tab-frame",
    spacing = 1,
})

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
    bar = "shaped"
})
