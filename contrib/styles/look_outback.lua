-- Authors: Mark Tran <mark@nirv.net>
-- License: Unknown
-- Last Changed: Unknown
--
-- look_outback.lua : Mark Tran <mark@nirv.net>

-- This style looks best when the alternative style is applied to "full"
-- workspace frames. You can enable it by applying: _:set_mode('tiled-alt')
-- as Lua code.
--
-- View http://modeemi.fi/~tuomov/ion/faq/entries/Hiding_the_tab-bar.html
-- for further details.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    border_style = "elevated",

    background_colour = "#453832",
    foreground_colour = "#ffffff",

    highlight_pixels = 0,
    padding_pixels = 0,
    shadow_pixels = 0,
    highlight_colour = "#000000",
    padding_colour = "#453832",
    shadow_colour = "#000000",

    font = "-xos4-terminus-*-r-normal--12-120-*-*-c-*-iso8859-1",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",

    border_style = "inlaid",
    spacing = 1,

    background_colour = "#f5deb3",
    foreground_colour = "#000000",

    padding_pixels = 1,
    padding_colour = "#453832",
})

de.defstyle("frame-floating", {
    bar = "inside",

    padding_pixels = 1,
    spacing = 0,
})

de.defstyle("frame-tiled-alt", {
    bar = "inside",
    spacing = 0,
})

de.defstyle("frame-transient", {
    spacing = 0,
})

de.defstyle("tab", {
    based_on = "*",

    highlight_pixels = 1,
    padding_pixels = 1,
    shadow_pixels = 1,
    highlight_colour = "#352722",
    shadow_colour = "#352722",

    spacing = 1,

    de.substyle("active-selected", {
        background_colour = "#983008",
        foreground_colour = "#ffffff",
        highlight_colour = "#d04008",
        shadow_colour = "#d04008",
    }),
    de.substyle("active-unselected", {
        background_colour = "#453832",
        foreground_colour = "#ffffff",
    }),
    de.substyle("inactive-selected", {
        background_colour = "#453832",
        foreground_colour = "#ffffff",
    }),
    de.substyle("inactive-unselected", {
        background_colour = "#453832",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("tab-frame-floating", {
    based_on = "tab",

    highlight_pixels = 0,
    shadow_pixels = 0,
})

de.defstyle("tab-frame-tiled-alt", {
    based_on = "tab",

    highlight_pixels = 0,
    shadow_pixels = 0,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",

    highlight_pixels = 0,
    padding_pixels = 3,
    shadow_pixels = 0,

    text_align = "left",

    de.substyle("inactive-selected", {
        background_colour = "#983008",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("tab-menuentry-big", {
    padding_pixels = 5,
})

de.defstyle("input", {
    background_colour = "#352722",
    padding_pixels = 2,
})

de.defstyle("input-edln", {
    background_colour = "#453832",

    highlight_pixels = 1,
    shadow_pixels = 1,
    highlight_colour = "#473e3b",
    shadow_colour = "#473e3b",

    de.substyle("*-cursor", {
        background_colour = "#ffffff",
        foreground_colour = "#000000",
    }),
    de.substyle("*-selection", {
        background_colour = "#983008",
        foreground_colour = "#ffffff",
    }),
})

de.defstyle("input-message", {
    background_colour = "#453832",
})

de.defstyle("stdisp-statusbar", {
    based_on = "*",

    de.substyle("important", {
        foreground_colour = "green",
    }),

    de.substyle("critical", {
        foreground_colour = "red",
    }),
})

de.defstyle("actnotify", {
    based_on = "*",

    highlight_pixels = 1,
    padding_pixels = 1,
    shadow_pixels = 1,
    background_colour = "#983008",
    foreground_colour = "#ffffff",
    highlight_colour = "#d04008",
    shadow_colour = "#d04008",
})

de.defstyle("moveres_display", {
    based_on = "actnotify",
})

gr.refresh()
