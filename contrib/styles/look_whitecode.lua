
if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    foreground_colour = "#000000",
    background_colour = "#f0f0f0",
    border_style = "ridge",
    padding_colour = "#c0c0c0",
    highlight_colour = "#000000",
    shadow_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 0,
    shadow_pixels = 0,
    spacing = 1,
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    spacing = 1,
    padding_pixels = 1,
})

de.defstyle("frame-tiled", {
    based_on = "frame",
})

de.defstyle("frame-floating", {
    based_on = "frame",
    floatframe_tab_min_w = 10000,
    floatframe_bar_max_w_q = 1,
    padding_colour = "#c0c0c0",
    de.substyle("active", {
      padding_colour = "black",
    })
})

de.defstyle("tab", {
    based_on = "*",
    border_style = "elevated",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    spacing = 0,
    text_align = "left",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("active-selected", {
        background_colour = "#ffffff",
        padding_colour = "#ffffff",
    }),
    de.substyle("active-unselected", {
        background_colour = "#e0e0e0",
        padding_colour = "#e0e0e0",
    }),
    de.substyle("inactive-selected", {
        background_colour = "#d0d0d0",
        padding_colour = "#d0d0d0",
        foreground_colour = "#808080",
        shadow_colour = "#d0d0d0",
        highlight_colour = "#d0d0d0",
    }),
    de.substyle("inactive-unselected", {
        background_colour = "#c0c0c0",
        foreground_colour = "#808080",
        padding_colour = "#c0c0c0",
        shadow_colour = "#c0c0c0",
        highlight_colour = "#c0c0c0",
    }),
})

de.defstyle("tab-frame-tiled", {
    based_on = "tab-frame",
    spacing = 1,
})

de.defstyle("tab-menuentry", {
    de.substyle("active-selected", {
        background_colour = "#e0e0e0",
        background_colour = "#b0c0d0",
        background_colour = "#505050",
        padding_colour = "#e0e0e0",
        padding_colour = "#b0c0d0",
        padding_colour = "#505050",
        foreground_colour = "#f0f0f0",
    }),
    de.substyle("active-unselected", {
        background_colour = "#ffffff",
        padding_colour = "#ffffff",
    }),
    de.substyle("inactive-selected", {
        background_colour = "#c0c0c0",
        foreground_colour = "#808080",
        padding_colour = "#c0c0c0",
    }),
    de.substyle("inactive-unselected", {
        background_colour = "#d0d0d0",
        padding_colour = "#d0d0d0",
        foreground_colour = "#808080",
    }),
    based_on = "tab",
    text_align = "left",
    border_style = "inlaid",
    highlight_pixels = 0,
    shadow_pixels = 0,
    spacing = 1,
    padding_pixels = 2,
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    border_style = "inlaid",
    padding_pixels = 5,
    spacing = 1,
})

de.defstyle("input", {
    based_on = "*",
    padding_pixels = 1,
    highlight_pixels = 0,
    shadow_pixels = 0,
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#ffffff",
    }),
    de.substyle("*-selection", {
        background_colour = "#c0c0c0",
        foreground_colour = "#000000",
    }),
})

de.defstyle("input-menu", {
    based_on = "*",
})

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 0,
    spacing = 0,
    background_colour = "#d0d0d0",
})

gr.refresh()

