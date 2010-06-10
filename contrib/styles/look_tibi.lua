-- $Id: look_tibi.lua 78 2007-02-07 18:36:32Z tibi $

-- version : 0.2
-- date    : 2007-02-07
-- author  : Tibor Csögör <tibi@tiborius.net>

-- This style highlights active elements with an `accent' color.  Bright and
-- dimmed variants emphasize the level of importance.  The corresponding neutral
-- colors are (roughly) the non-saturated versions.

-- The author likes the color scheme `gold' best, however, feel free to
-- experiment with the accent color(s).

-- This software is in the public domain.


-- color configuration ---------------------------------------------------------

-- gold
local my_accent_color_bright = "lightgoldenrod1"
local my_accent_color_normal = "gold1"
local my_accent_color_dimmed = "gold2"
local my_accent_color_dark   = "gold3"

-- green
-- local my_accent_color_bright = "#c2ffc2"
-- local my_accent_color_normal = "palegreen1"
-- local my_accent_color_dimmed = "palegreen2"
-- local my_accent_color_dark   = "palegreen3"

-- blue
-- local my_accent_color_bright = "lightblue1"
-- local my_accent_color_normal = "skyblue1"
-- local my_accent_color_dimmed = "skyblue2"
-- local my_accent_color_dark   = "skyblue3"

-- plum
-- local my_accent_color_bright = "#ffd3ff"
-- local my_accent_color_normal = "plum1"
-- local my_accent_color_dimmed = "plum2"
-- local my_accent_color_dark   = "plum3"

--------------------------------------------------------------------------------


-- neutral colors
local my_neutral_color_normal = "grey85"
local my_neutral_color_dimmed = "grey70"
local my_neutral_color_dark   = "grey20"


if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    padding_pixels = 0,
    spacing = 0,
    foreground_colour = "black",
    background_colour = my_accent_color_bright,
    highlight_pixels = 1,
    highlight_colour = "black",
    shadow_pixels = 1,
    shadow_colour = "black",
    border_style = "elevated",
})

de.defstyle("frame", {
    based_on = "*",
    background_colour = "black",
})

de.defstyle("frame-floating", {
    based_on = "frame",
    padding_pixels = 0,
    highlight_pixels = 3,
    highlight_colour = my_neutral_color_normal,
    shadow_pixels = 3,
    shadow_colour = my_neutral_color_normal,
    de.substyle("active", {
    	highlight_colour = my_accent_color_normal,
        shadow_colour = my_accent_color_normal,
    }),
})

de.defstyle("frame-tiled", {
    based_on = "frame",
    spacing = 1,
    padding_pixels = 3,
    highlight_pixels = 2,
    highlight_colour = my_neutral_color_dark,
    shadow_pixels = 2,
    shadow_colour = my_neutral_color_dark,
    de.substyle("active", {
    	highlight_colour = my_accent_color_normal,
        shadow_colour = my_accent_color_normal,
    }),
})

de.defstyle("tab", {
    based_on = "*",
    spacing = 2,
    padding_pixels = 2,
    highlight_pixels = 0,
    shadow_pixels = 0,
    foreground_colour = "black",
    background_colour = my_neutral_color_dimmed,
    text_align = "center",
    font = "-*-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*",
    de.substyle("active-selected", {
        background_colour = my_accent_color_normal,
    }),
    de.substyle("active-unselected", {
        background_colour = my_accent_color_dark,
    }),
    de.substyle("inactive-selected", {
        background_colour = my_neutral_color_normal,
    }),
})

de.defstyle("input-edln", {
    based_on = "*",
    padding_pixels = 2,
    foreground_colour = "black",
    background_colour = my_accent_color_bright,
    highlight_pixels = 1,
    highlight_colour = my_accent_color_dark,
    shadow_pixels = 1,
    shadow_colour = my_accent_color_dark,
    font ="-*-lucidatypewriter-medium-r-*-*-10-*-*-*-*-*-*-*",
    de.substyle("*-cursor", {
        foreground_colour = my_accent_color_bright,
        background_colour = "black",
    }),
    de.substyle("*-selection", {
        foreground_colour = "black",
        background_colour = my_accent_color_dimmed,
    }),
})

de.defstyle("stdisp", {
    based_on = "*",
    padding_pixels = 4,
    shadow_pixels = 0,
    highlight_pixels = 0,
    background_colour = my_neutral_color_dark,
    foreground_colour = "white",
    font ="-*-lucida-medium-r-*-*-10-*-*-*-*-*-*-*",
})

de.defstyle("tab-menuentry", {
    based_on = "*",
    text_align = "left",
    padding_pixels = 4,
    spacing = 0,
    shadow_pixels = 0,
    highlight_pixels = 0,
    font = "-*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*",
    foreground_colour = "black",
    background_colour = my_accent_color_bright,
    de.substyle("inactive-selected", {
        background_colour = my_accent_color_dimmed,
    }),
})

de.defstyle("actnotify", {
    based_on = "*",
    padding_pixels = 4,
    highlight_pixels = 2,
    highlight_colour = my_accent_color_normal,
    shadow_pixels = 2,
    shadow_colour = my_accent_color_normal,
    background_colour = "red",
    foreground_colour = "white",
    font = "-*-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*",
})

gr.refresh()

-- EOF
