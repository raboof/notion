-- Settings common to some styles.

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    text_align = "left",

    de.substyle("important", {
        foreground_colour = "green",
    }),

    de.substyle("critical", {
        foreground_colour = "red",
    }),
})

de.defstyle("actnotify", {
    based_on = "*",
    shadow_colour = "#401010",
    highlight_colour = "#907070",
    background_colour = "#990000",
    foreground_colour = "#eeeeee",
})
