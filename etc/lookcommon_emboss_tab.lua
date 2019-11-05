-- Common tab settings for the "emboss" styles

de.defstyle("actnotify", {
    shadow_colour = "#600808",
    highlight_colour = "#c04040",
    background_colour = "#b03030",
    foreground_colour = "#ffffff",
})

de.defstyle("tab-frame", {
    -- TODO: some kind of amend option. It should not be necessary to
    -- duplicate this definition for both tab-frame and tab-menuentry,
    -- or for each style, nor use more complex hacks to communicate
    -- this stuff otherwise.
    de.substyle("*-*-*-unselected-activity", {
        shadow_colour = "#600808",
        highlight_colour = "#c04040",
        background_colour = "#901010",
        foreground_colour = "#eeeeee",
    }),

    de.substyle("*-*-*-selected-activity", {
        shadow_colour = "#600808",
        highlight_colour = "#c04040",
        background_colour = "#b03030",
        foreground_colour = "#ffffff",
    }),

    de.substyle("*-*-*-tabnumber", {
        background_colour = "black",
        foreground_colour = "green",
    }),
})

de.defstyle("tab-frame-tiled", {
    spacing = 1,
})

de.defstyle("tab-menuentry", {
    text_align = "left",
    highlight_pixels = 0,
    shadow_pixels = 0,

    de.substyle("*-*-*-unselected-activity", {
        shadow_colour = "#600808",
        highlight_colour = "#c04040",
        background_colour = "#901010",
        foreground_colour = "#eeeeee",
    }),

    de.substyle("*-*-*-selected-activity", {
        shadow_colour = "#600808",
        highlight_colour = "#c04040",
        background_colour = "#b03030",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("tab-menuentry-big", {
    padding_pixels = 7,
})
