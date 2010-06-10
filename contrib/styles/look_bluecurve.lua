-- look_bluecurve.lua drawing engine configuration file for Ion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "#404040",
    highlight_colour = "#707070",
    background_colour = "#e6e6e6",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    shadow_colour = "#ffffff",
    highlight_colour = "#a1a1a1",
    padding_colour = "#e6e6e6",
    background_colour = "#e6e6e6",
    foreground_colour = "#ffffff",
    padding_pixels = 2,
    highlight_pixels = 1,
    shadow_pixels = 1,
})

de.defstyle("frame-tiled", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
    spacing = 1,
})

de.defstyle("frame-floating", {
    based_on = "frame",
    border_style = "ridge"
})

de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "#404679",
        highlight_colour = "#c0c6f9",
        background_colour = "#8086b9",
        foreground_colour = "#ffffff",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#a1a1a1",
        highlight_colour = "#ffffff",
        background_colour = "#e6e6e6",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-selected", {
		shadow_colour = "#a1a1a1",
        highlight_colour = "#ffffff",
        background_colour = "#e6e6e6",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#ffffff",
        highlight_colour = "#a1a1a1",
        background_colour = "#e6e6e6",
        foreground_colour = "#a0a0a0",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#404040",
        highlight_colour = "#707070",
        background_colour = "#990000",
        foreground_colour = "#eeeeee",
    }),
})

de.defstyle("tab-frame-tiled", {
    based_on = "tab-frame",
    spacing = 1,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    highlight_pixels = 0,
    shadow_pixels = 0,
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "#404040",
    highlight_colour = "#707070",
    background_colour = "#e6e6e6",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "#ffffff",
        foreground_colour = "#000000",
    }),
    de.substyle("*-selection", {
        background_colour = "#505050",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("input-menu", {
    based_on = "*",
    de.substyle("active", {
        shadow_colour = "#304050",
        highlight_colour = "#708090",
        background_colour = "#506070",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    text_align = "left",
    --font = "fixed",

    de.substyle("important", {
        foreground_colour = "green",
    }),

    de.substyle("critical", {
        foreground_colour = "red",
    }),
})

gr.refresh()

