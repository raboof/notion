-- look-bb.lua for use with the bb background.
-- Bas Kok 20040916 

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "#404040",
    highlight_colour = "#707070",
    background_colour = "#505050",
    foreground_colour = "#a0a0a0",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "-artwiz-snap-*-*-*-*-*-100-*-*-*-*-*-*",
    text_align = "center",
    transparent_background = true,
})

de.defstyle("frame", {
    based_on = "*",
    shadow_colour = "#404040",
    highlight_colour = "#707070",
    padding_colour = "#505050",
    background_colour = "#1c2636",
    foreground_colour = "#b6b4b8",
    padding_pixels = 0,
    highlight_pixels = 0,
    shadow_pixels = 0,
    de.substyle("active", {
        shadow_colour = "#0f1729",
        highlight_colour = "#637782",
        padding_colour = "#4b7d96",
        foreground_colour = "#b6b4b8",
    }),
    transparent_background = true,
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 0,
    spacing = 0,
})

de.defstyle("frame-floatframe", {
    based_on = "frame",
    border_style = "ridge"
})

de.defstyle("tab", {
    based_on = "*",
    font = "-artwiz-snap-*-*-*-*-*-100-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "#435663",
        highlight_colour = "#435663",
        background_colour = "#546b7c",
        foreground_colour = "#b6b4b8",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#435663",
        highlight_colour = "#435663",
        background_colour = "#435663",
        foreground_colour = "#a0a0a0",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#161d21",
        highlight_colour = "#161d21",
        background_colour = "#232c33",
        foreground_colour = "#a0a0a0",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#161d21",
        highlight_colour = "#161d21",
        background_colour = "#161d21",
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

de.defstyle("tab-frame-ionframe", {
    based_on = "tab-frame",
    spacing = 0,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    highlight_pixels = 0,
    shadow_pixels = 0,
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-artwiz-snap-*-*-*-*-*-100-*-*-*-*-*-*",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "#404040",
    highlight_colour = "#707070",
    background_colour = "#1c2636",
    foreground_colour = "#b6b4b8",
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "#b6b4b8",
        foreground_colour = "#1c2636",
    }),
    de.substyle("*-selection", {
        background_colour = "#505050",
        foreground_colour = "#b6b4b8",
    }),
})

de.defstyle("input-menu", {
    based_on = "*",
    de.substyle("active", {
        shadow_colour = "#0f1729",
        highlight_colour = "#637782",
        background_colour = "#5892b0",
        foreground_colour = "#b6b4b8",
    }),
})

gr.refresh()

