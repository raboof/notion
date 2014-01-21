-- Common settings for the "emboss" styles

de.defstyle("frame", {
    background_colour = "#000000",
    -- The special "inherit" value causes setting 'background_colour'
    -- above not to set padding_colour, but this colour being inherited.
    padding_colour = "inherit",
    de.substyle("quasiactive", {
        -- Something detached from the frame is active
        padding_colour = "#901010",
    }),
    de.substyle("userattr1", {
        -- For user scripts
        padding_colour = "#009010",
    }),
    border_style = "ridge",
    padding_pixels = 2,
    highlight_pixels = 1,
    shadow_pixels = 1,
    spacing = 1,
})

de.defstyle("frame-tiled", {
    border_style = "inlaid",
    padding_pixels = 1,
})

--de.defstyle("frame-tiled-alt", {
--    bar = "none",
--})

de.defstyle("frame-floating", {
    --bar = "shaped",
    spacing = 0,
})

de.defstyle("frame-transient", {
    --bar = "none",
    spacing = 0,
})

de.defstyle("actnotify", {
    shadow_colour = "#600808",
    highlight_colour = "#c04040",
    background_colour = "#b03030",
    foreground_colour = "#ffffff",
})

de.defstyle("tab", {
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


de.defstyle("tab-frame", {
    spacing = 1,
})

de.defstyle("tab-frame-floating", {
    spacing = 0,
})

de.defstyle("tab-frame-transient", {
    spacing = 0,
})

de.defstyle("tab-menuentry", {
    text_align = "left",
    highlight_pixels = 0,
    shadow_pixels = 0,
})

de.defstyle("tab-menuentry-big", {
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})


de.defstyle("stdisp", {
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
