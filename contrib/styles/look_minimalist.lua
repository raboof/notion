-- look_minimalist.lua drawing engine configuration file for Ion.

--one basic style motivated by the need to work with min_tabs.lua
--borders are slightly wider, and active-selected colour is much more
--vivid so it's easy to tell which frame has the focus even without tabs
--also, use relatively small font for the tabs -- which are always on
--screen -- but use a much bigger & easier on the eye font for pop-up
--menus which only take up space on screen transiently

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
    shadow_colour = "#606060",
    highlight_colour = "#ffffff",
    background_colour = "#d8d8d8",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    border_style = "elevated",
    font = "-*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*",
    text_align = "center",
})

de.defstyle("frame", {
    based_on = "*",
    shadow_colour = "#404000",
    highlight_colour = "#c0c000",
    padding_colour = "#808000",
    background_colour = "#000000",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    de.substyle("active", {
        shadow_colour = "#ff0000",
        highlight_colour = "#ff0000",
        padding_colour = "#ff0000",
        background_colour = "#d8d8d8",
        foreground_colour = "#000000",
    }),
})

de.defstyle("frame-ionframe", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
    spacing = 0,
})

de.defstyle("frame-floatframe", {
    based_on = "frame",
    border_style = "ridge"
})

de.defstyle("tab", {
    based_on = "*",
    font = "-*-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*",
    padding_pixels = 0,
    highlight_pixels = 0,
    shadow_pixels = 0,
    de.substyle("active-selected", {
        shadow_colour = "#df2900",
        highlight_colour = "#ff5900",
        background_colour = "#ff3900",
        foreground_colour = "#000000",
    }),
    de.substyle("active-unselected", {
        shadow_colour = "#dfb700",
        highlight_colour = "#ffe700",
        background_colour = "#ffc700",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-selected", {
        shadow_colour = "#6b2900",
        highlight_colour = "#ab6900",
        background_colour = "#8b4900",
        foreground_colour = "#000000",
    }),
    de.substyle("inactive-unselected", {
        shadow_colour = "#6b5500",
        highlight_colour = "#ab9500",
        background_colour = "#8b7500",
        foreground_colour = "#000000",
    }),
    text_align = "center",
})

de.defstyle("tab-frame", {
    based_on = "tab",
    de.substyle("*-*-*-*-activity", {
        shadow_colour = "#600000",
        highlight_colour = "#ff0000",
        background_colour = "#32bc32",
        foreground_colour = "#ff0000",
    }),
})

de.defstyle("tab-frame-ionframe", {
    based_on = "tab-frame",
    spacing = 0,
})

de.defstyle("tab-menuentry", {
    based_on = "tab",
    font = "-*-helvetica-medium-r-normal-*-24-*-*-*-*-*-*-*",
    text_align = "left",
    highlight_pixels = 0,
    shadow_pixels = 0,
    --make tab menus bright rather than drab even with inactive tabs 
    de.substyle("inactive-unselected", {
       shadow_colour = "#dfb700",
       highlight_colour = "#ffe700",
       background_colour = "#ffc700",
       foreground_colour = "#000000",
    }),
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = "-*-helvetica-medium-r-normal-*-24-*-*-*-*-*-*-*",
    padding_pixels = 7,
    de.substyle("inactive-unselected", {
        shadow_colour = "#dfb700",
        highlight_colour = "#ffe700",
        background_colour = "#ffc700",
        foreground_colour = "#000000",
    }),
})

de.defstyle("input", {
    based_on = "*",
    shadow_colour = "#606060",
    highlight_colour = "#ffffff",
    background_colour = "#d8d8d8",
    foreground_colour = "#000000",
    padding_pixels = 1,
    highlight_pixels = 1,
    shadow_pixels = 1,
    font = "-*-helvetica-medium-r-normal-*-24-*-*-*-*-*-*-*",
    border_style = "elevated",
    de.substyle("*-cursor", {
        background_colour = "#000000",
        foreground_colour = "#d8d8d8",
    }),
    de.substyle("*-selection", {
        background_colour = "#f0c000",
        foreground_colour = "#000000",
    }),
})

de.defstyle("stdisp", {
    based_on = "*",
    shadow_pixels = 0,
    highlight_pixels = 0,
    text_align = "left",
})

gr.refresh()

