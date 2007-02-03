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
    shadow_colour = "#600808",
    highlight_colour = "#c04040",
    background_colour = "#b03030",
    foreground_colour = "#ffffff",
})

de.defstyle("tab-frame", {
    based_on = "tab",
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
})

de.defstyle("tab-frame-tiled", {
    based_on = "tab-frame",
    spacing = 1,
})

de.defstyle("frame-tiled", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
    spacing = 1,
})

de.defstyle("frame-floating", {
    based_on = "frame",
    border_style = "ridge",
    bar = "shaped"
})

de.defstyle("frame-tiled-alt", {
    based_on = "frame-tiled",
    bar = "none",
})

de.defstyle("tab-menuentry", {
    based_on = "tab",
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
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})
