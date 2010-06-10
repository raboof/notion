
if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "#00aa00",
    highlight_colour = "#00aa00",
--    background_colour = "#000000",
    foreground_colour = "#00aa00",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    shadow_colour = "#000000",
    highlight_colour = "#000000",
    padding_colour = "#00aa00",
--    transparent_background = true,
--    background_colour = "#000000",
    foreground_colour = "#ffffff",
    padding_pixels = 2,
    highlight_pixels = 1,
    shadow_pixels = 1,
    de.substyle("active", {
        shadow_colour = "#000000",
        highlight_colour = "#000000",
        padding_colour = "#00aa00",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
    spacing = 1,
})

de.defstyle("frame-floatframe", {
    based_on = "frame",
    border_style = "ridge",
})

de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "#00aa00",
        highlight_colour = "#00aa00",
        background_colour = "#000000",
        foreground_colour = "#00aa00",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#006600",
        highlight_colour = "#006600",
        background_colour = "#000000",
        foreground_colour = "#006600",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#006600",
        highlight_colour = "#006600",
        background_colour = "#000000",
        foreground_colour = "#006600",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#003300",
        highlight_colour = "#003300",
        background_colour = "#000000",
        foreground_colour = "#003300",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#00aa00",
        highlight_colour = "#00aa00",
        background_colour = "#000000",
        foreground_colour = "#00aa00",
    }),
})

de.defstyle("tab-frame-ionframe", {
    based_on = "tab-frame",
    spacing = 1,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    highlight_pixels = 0,
    shadow_pixels = 0,
    --give pmenus the same brightness as the other menus.
    de.substyle("inactive-selected", {
        shadow_colour = "#00aa00",
        highlight_colour = "#00aa00",
        background_colour = "#000000",
        foreground_colour = "#00aa00",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#006600",
        highlight_colour = "#006600",
        background_colour = "#000000",
        foreground_colour = "#006600",
    }),
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "#00aa00",
    highlight_colour = "#00aa00",
    background_colour = "#000000",
    foreground_colour = "#00aa00",
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
        shadow_colour = "#00aa00",
        highlight_colour = "#00aa00",
        background_colour = "#000000",
        foreground_colour = "#00aa00",
    }),
})

gr.refresh()

