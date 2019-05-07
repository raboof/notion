-- Authors: Steve Pomeroy <steve@staticfree.info>
-- License: Unknown
-- Last Changed: Unknown
--
-- look-cleansteel_trans.lua drawing engine configuration file for Ion.

-- Based on the default cleansteel look, but with added transparency
-- and a few other things that I like. -Steve Pomeroy <steve@staticfree.info>

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
    font = "-*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*",
    text_align = "center",
    transparent_background = false,
})

de.defstyle("frame", {
    based_on = "*",
    --shadow_colour = "#404040",
    --highlight_colour = "#707070",
    padding_colour = "#505050",
    background_colour = "#000000",
    foreground_colour = "#ffffff",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    --de.substyle("active", {
    --    shadow_colour = "#203040",
    --    highlight_colour = "#607080",
    --    padding_colour = "#405060",
    --    foreground_colour = "#ffffff",
    --}),
    border_style = "ridge",
    transparent_background = true,
})

de.defstyle("frame-tiled", {
    based_on = "frame",
    spacing = 1,
    padding_pixels = 0,
    highlight_pixels = 0,
    shadow_pixels = 0,
})

--de.defstyle("frame-floatframe", {
--    based_on = "frame",
--})

de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        shadow_colour = "#304050",
        highlight_colour = "#708090",
        background_colour = "#506070",
        foreground_colour = "#ffffff",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#203040",
        highlight_colour = "#607080",
        background_colour = "#405060",
        foreground_colour = "#a0a0a0",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#404040",
        highlight_colour = "#909090",
        background_colour = "#606060",
        foreground_colour = "#a0a0a0",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#404040",
        highlight_colour = "#707070",
        background_colour = "#505050",
        foreground_colour = "#a0a0a0",
    }),
    text_align = "center",
    transparent_background = true,
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
    font = "-*-helvetica-medium-r-normal-*-*-*-*-*-*-*-*-*",
    padding_pixels = 7,
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "#404040",
    highlight_colour = "#707070",
    background_colour = "#000030",
    foreground_colour = "#ffffff",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    transparent_background = false,

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

de.defstyle("input-edln", {
    based_on = "input",
    -- this is taken from the URxvt default font on my system. -SP
    font = "-misc-fixed-medium-r-semicondensed--13-*-*-*-c-60-*-*",
})

de.defstyle("input-menu", {
    based_on = "*",
    transparent_background = true,

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
    background_colour = "#000000",
    foreground_colour = "grey",
    text_align = "left",

    de.substyle("important", {
        foreground_colour = "green",
    }),

    de.substyle("critical", {
        foreground_colour = "red",
    }),

    de.substyle("actnotify", {
        foreground_colour = "red",
    }),
})

de.defstyle("actnotify", {
	based_on = "*",
        shadow_colour = "#404040",
        highlight_colour = "#707070",
        background_colour = "#990000",
	foreground_colour = "#eeeeee",
})

gr.refresh()

