-- Look-qtrc 0.1 for the Ion window manager.
-- Based on look-cleanviolet.
--
-- I am using the QtCurve Gtk+ theme; now Qt, Gtk+ and Ion share the same
-- color theme automatically. :-)
--
-- http://www.kde-look.org/content/show.php?content=5065
--
-- Johan "Ion" Kiviniemi

local xft = false
local basefont
local tabfont
local menufont
--if gr.select_engine("xftde") then
if false then
    xft = true
    basefont = "xft:Bitstream Vera Sans:size=9:weight=bold"
    tabfont = "xft:Bitstream Vera Sans:size=8:weight=bold"
    menufont = "xft:Bitstream Vera Sans:size=10:weight=bold"
elseif gr.select_engine("de") then
    xft = false
    basefont = "-*-bitstream vera sans-bold-r-normal-*-9-*-*-*-*-*-*-*"
    font = "-*-bitstream vera sans-bold-r-normal-*-8-*-*-*-*-*-*-*"
    menufont = "-*-bitstream vera sans-bold-r-normal-*-10-*-*-*-*-*-*-*"
else
    return
end

local qtrcfile = os.getenv("HOME").."/.qt/qtrc"

local palette = {}
-- palette[1] = active, palette[2] = inactive, palette[3] = disabled
for i = 1, 3 do
    palette[i] = {}
    for j = 1, 16 do
        palette[i][j] = "#000000"
    end
end

local inpalette = false
local paletteid = 0
for line in io.lines(qtrcfile) do
    if string.find(line, "^%[") then inpalette = false end
    if string.find(line, "^%[Palette%]") then inpalette = true end
    if inpalette then
        if string.find(line, "^active=") then paletteid = 1
        elseif string.find(line, "^inactive=") then paletteid = 2
        elseif string.find(line, "^disabled=") then paletteid = 3
        else paletteid = 0
        end

        if paletteid > 0 then
            local i = 1
            for v in string.gfind(line, "(#[0-9a-fA-F]+)") do
                palette[paletteid][i] = v
                i = i+1
            end
        end
    end
end

-- Clear existing styles from memory.
de.reset()

-- Base style
de.defstyle("*", {
    highlight_colour = palette[1][4],
    shadow_colour = palette[1][5],
    background_colour = palette[1][2],
    foreground_colour = palette[1][1],

    shadow_pixels = 1,
    highlight_pixels = 1,
    padding_pixels = 1,
    spacing = 0,
    border_style = "elevated",

    font = basefont,
    text_align = "left",
})


de.defstyle("frame", {
    based_on = "*",
    padding_colour = palette[1][2],
    background_colour = palette[1][1],
    transparent_background = false,
})


de.defstyle("frame-ionframe", {
    based_on = "frame",
    shadow_pixels = 0,
    highlight_pixels = 0,
    padding_pixels = 0,
    spacing = 1,
})


de.defstyle("tab", {
    based_on = "*",
    font = tabfont,

    de.substyle("active-selected", {
        highlight_colour = palette[1][13],
        shadow_colour = palette[1][13],
        background_colour = palette[1][13],
        foreground_colour = palette[1][14],
    }),

    de.substyle("inactive-selected", {
        highlight_colour = palette[1][4],
        shadow_colour = palette[1][5],
        background_colour = palette[1][2],
        foreground_colour = palette[1][1],
    }),
})


de.defstyle("tab-frame", {
    based_on = "tab",

    de.substyle("*-*-*-*-activity", {
        -- Red tab
        highlight_colour = "#ffffff",
        shadow_colour = "#ffffff",
        background_colour = "#990000",
        foreground_colour = "#ffffff",
    }),
})


de.defstyle("tab-frame-ionframe", {
    based_on = "tab-frame",
    spacing = 1,
    bar_inside_frame = true,
})


de.defstyle("tab-menuentry", {
    based_on = "tab",
    text_align = "left",
    spacing = 1,
})

de.defstyle("tab-menuentry-pmenu", {
    based_on = "tab-menuentry",
    de.substyle("inactive-selected", {
        highlight_colour = palette[1][13],
        shadow_colour = palette[1][13],
        background_colour = palette[1][13],
        foreground_colour = palette[1][14],
    }),
})

de.defstyle("tab-menuentry-big", {
    based_on = "tab-menuentry",
    font = menufont,
    padding_pixels = 5,
})


de.defstyle("input", {
    based_on = "*",
    text_align = "left",
    spacing = 1,
    highlight_colour = palette[1][4],
    shadow_colour = palette[1][5],
    background_colour = palette[1][2],
    foreground_colour = palette[1][1],

    de.substyle("*-selection", {
        background_colour = "#0000ff",
        foreground_colour = "#00ff00",
    }),

    de.substyle("*-cursor", {
        background_colour = palette[1][1],
        foreground_colour = palette[1][2],
    }),
})

-- Refresh objects' brushes.
gr.refresh()
