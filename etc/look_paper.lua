-- look_paper
-- tabs without borders

if not gr.select_engine("de") then
    return
end


-- Clear existing styles from memory.
de.reset()

local empty = "#212026"
local padding = 3
local tab_line = 2
decoration = {
  active = {
      background = "#5d4d7a"
    , foreground  = { default = "grey", selected = "#ffffff"}
  }
  , inactive = {
      background = "#212026"
      , foreground  = { default = "#9aa28b", selected = "#eeeeee" }
      , sp = {
          background = "#1d4d7a"
          , foreground  = { default = "grey", selected = "#ffffff" }
      }
  }
  , preview = {
      background = "#4E4B59"
      , foreground  = { default = "grey", selected = "#ffffff" }
  }
}

-- Base decoration
de.defstyle("*", {
                -- Gray background
                highlight_colour = "#eeeeee",
                shadow_colour = "#eeeeee",
                background_colour = empty,
                foreground_colour = "#9aa28b",
                shadow_pixels = 1,
                highlight_pixels = 1,
                padding_pixels = 1,
                -- spacing = 0,
                border_style = "elevated",

                font = "-*-noto-medium-r-normal-*-14-*-*-*-*-*-*-*",
                text_align = "center",
})

de.defstyle("frame-tiled", {
                border_sides = "lr",
                border_style = "elevated",
                shadow_pixels = padding,
                highlight_pixels = padding,
                padding_pixels = 0,
                spacing = 0,
                highlight_colour = decoration.inactive.background,
                shadow_colour    = decoration.inactive.background,
                background_colour = empty,

                de.substyle("inactive-*-preview", {
                                background_colour = decoration.preview.background,
                                highlight_colour = decoration.preview.background,
                                shadow_colour    = decoration.preview.background,
                }),

                de.substyle("active", {
                              background_colour = decoration.active.background,
                              highlight_colour = decoration.active.background,
                              shadow_colour    = decoration.active.background,
                }),
                de.substyle("inactive-selected", {
                              highlight_colour = decoration.inactive.background,
                              shadow_colour    = decoration.inactive.background,
                })
})

de.defstyle("tab", {
    font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
    show_icon=true,
    icon_align_left=false,
    border_sides="tb",
    border_style="elevated",
    shadow_pixels=tab_line,
    highlight_pixels=0,
    spacing = 0,

    shadow_colour = decoration.inactive.foreground.default,
    highlight_colour = decoration.inactive.foreground.default,
    background_colour = decoration.inactive.background,

    de.substyle("inactive-selected-preview", {
                    background_colour = decoration.preview.background,
                    shadow_colour = decoration.preview.foreground.selected,
                    foreground_colour = decoration.preview.foreground.selected,
    }),

    de.substyle("inactive-preview", {
                    background_colour = decoration.preview.background,
                    shadow_colour = decoration.preview.foreground.default,
                    foreground_colour = decoration.preview.foreground.default,
    }),

    de.substyle("active", {
        background_colour = decoration.active.background,
        shadow_colour = decoration.active.foreground.default,
        foreground_colour = decoration.active.foreground.default,
    }),

    de.substyle("active-selected", {
        background_colour = decoration.active.background,
        shadow_colour = decoration.active.foreground.selected,
        foreground_colour = decoration.active.foreground.selected,
    }),

    de.substyle("inactive-selected", {
        background_colour = decoration.inactive.background,
        shadow_colour = decoration.inactive.foreground.selected,
        foreground_colour = decoration.inactive.foreground.selected
    }),

    -- urgent hint style
    de.substyle("inactive-*-*-unselected-activity", {
                  shadow_colour = "#b03030",
                    background_colour = decoration.inactive.background,
                    foreground_colour = decoration.inactive.foreground.default,
}),
    de.substyle("inactive-*-*-selected-activity", {
                  shadow_colour = "#fe0f0f",
                  background_colour = decoration.inactive.background,
                  foreground_colour = decoration.inactive.foreground.selected,
    }),

    de.substyle("active-*-*-unselected-activity", {
                  shadow_colour = "#fe0f0f",
                  background_colour = decoration.active.background,
                  foreground_colour = decoration.active.foreground.default,
    }),
})

de.defstyle("tab-frame-floating", {
              border_style = "elevated",
              padding_pixels = 0,
              highlight_pixels = 0,
              shadow_pixels = 2,
              border_sides = "tb",
              spacing = 0
})


de.defstyle("input", {
  text_align = "left",
	font = "-*-fixed-medium-*-normal-*-15-*-*-*-*-*-iso10646-*",
  spacing = 0,
	border_style = "elevated",
	border_sides = "tb",
    -- Greyish violet background
    highlight_colour = "#9a9292",
    shadow_colour = "#000000",
    background_colour = "#101028",
    foreground_colour = "#ffffff",
    
    de.substyle("*-selection", {
        background_colour = "#9a9292",
        foreground_colour = "#000000",
    }),

    de.substyle("*-cursor", {
        background_colour = "#ffffff",
        foreground_colour = "#9999aa",
    }),
})

de.defstyle("tab-menuentry", {
                highlight_pixels = 0,
                shadow_pixels = 0,
                padding_pixels = 1,
                text_align = "left",
})

de.defstyle("tab-menuentry-big", {
                font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
                padding_pixels = 7,
})


de.defstyle("actnotify", {
                shadow_colour = "#c04040",
                highlight_colour = "#c04040",
                background_colour = "#901010",
                foreground_colour = "#eeeeee",
})


de.defstyle("stdisp", {
                border_sides = "tb",
                border_style = "elevated",
                shadow_pixels = 0,
                highlight_pixels = 2,
                highlight_colour = decoration.inactive.foreground.default,
                text_align = "left",
                background_colour = empty,
                foreground_colour = "grey",
                font="-misc-fixed-medium-r-*-*-13-*-*-*-*-60-*-*",

                de.substyle("important", {
                                foreground_colour = "green",
                }),

                de.substyle("critical", {
                                foreground_colour = "red",
                }),
})

de.defstyle("frame-floating-alt", {
              bar = "none",
              border_sides = "all",
              -- padding_pixels = 2,
              highlight_pixels = 8,
              shadow_pixels = 8,
              highlight_colour =
                decoration.inactive.sp.foreground.default,
              shadow_colour =
                decoration.inactive.sp.foreground.default,
              highlight_colour = decoration.inactive.sp.background,
              shadow_colour    = decoration.inactive.sp.background,
})

de.defstyle("frame-unknown-alt", {
              bar = "none",
              border_sides = "all",
              spacing = 0,
              padding_pixels = 0,
              highlight_pixels = 0,
              shadow_pixels = 0
})

-- Refresh objects' brushes.
gr.refresh()
