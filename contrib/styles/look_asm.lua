
if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    foreground_colour = "#ffffff",
    background_colour = "#708090",
    shadow_colour = "#405060",
    highlight_colour = "#708090",
    padding_colour= "#405060",

    padding_pixels = 0,
    highlight_pixels = 1,
    shadow_pixels = 0,
    spacing = 0,
    -- border_style = "elevated",
    border_style = "ridge",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    background_colour = "#040810",
    shadow_colour = "#405060",
    highlight_colour = "#708090",
    padding_colour= "#708090",
    shadow_pixels = 1,
    highlight_pixels = 1,
    spacing = 1,
    padding_pixels = 1,
})

de.defstyle("frame-tiled", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 0,
    --highlight_pixels = 0,
    spacing = 0,
    shadow_pixels = 0,
    highlight_pixels = 1,
})

de.defstyle("frame-floating", {
    based_on = "frame",
    shadow_colour = "#405060",
    highlight_colour = "#708090",
    padding_pixels = 0,
    shadow_pixels = 1,
    highlight_pixels = 1,
    spacing = 0,
    border_style = "elevated",
    floatframe_tab_min_w = 10000,
    floatframe_bar_max_w_q = 1,
})

de.defstyle("tab", {
    based_on = "*",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#506070",
        highlight_colour = "#506070",
        background_colour = "#993020",
        padding_colour= "#993020",
        foreground_colour = "#eeeeee",
    }),
    de.substyle("active-selected", {
        shadow_colour = "#708090",
        highlight_colour = "#708090",
        background_colour = "#607080",
        padding_colour= "#607080",
        foreground_colour = "#ffffff",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#607080",
        highlight_colour = "#607080",
        background_colour = "#405060",
        padding_colour= "#405060",
        foreground_colour = "#a0a0a0",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#607080",
        highlight_colour = "#607080",
        background_colour = "#405060",
        padding_colour= "#405060",
        foreground_colour = "#c0c0c0",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#506070",
        highlight_colour = "#506070",
        background_colour = "#304050",
        padding_colour= "#304050",
        foreground_colour = "#a0a0a0",
    }),
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    border_style = "ridge",
    padding_pixels = 0,
    shadow_pixels = 0,
    highlight_pixels = 1,
    spacing = 1,
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    spacing = 0,
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
    spacing = 1,
    padding_pixels = 0,
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    border_style = "inlaid",
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 5,
    spacing = 1,
})

de.defstyle("input", {
    based_on = "*",
    background_colour = "#304050",
    padding_pixels = 1,
    highlight_pixels = 0,
    shadow_pixels = 0,
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "#ffffff",
        foreground_colour = "#000000",
    }),
    de.substyle("*-selection", {
        background_colour = "#506070",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("input-menu", {
    based_on = "*",
    padding_colour = "#607080",
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
    padding_pixels = 1,
    spacing = 1,
    text_align = "center",
    background_colour = "#506070",
})

gr.refresh()

