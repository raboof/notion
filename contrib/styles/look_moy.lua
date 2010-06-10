-- look-greyviolet.lua drawing engine configuration file for Ion.

if not gr.select_engine("de") then return end

de.reset()

de.defstyle("*", {
                   shadow_colour = "#777777",
                   highlight_colour = "#eeeeee",
                   background_colour = "#aaaaaa",
                   foreground_colour = "#000000",
                   padding_pixels = 1,
                   highlight_pixels = 1,
                   shadow_pixels = 1,
                   border_style = "elevated",
                   font = "-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*",
                   text_align = "center",
                })

de.defstyle("frame", {
                   based_on = "*",
                   shadow_colour = "#777777",
                   highlight_colour = "#dddddd",
                   padding_colour = "#aaaaaa",
                   background_colour = "#000000",
                   foreground_colour = "#ffffff",
                   padding_pixels = 1,
                   highlight_pixels = 1,
                   shadow_pixels = 1,
                   de.substyle("active", {
                                  border_style = "inlaid",
                                  shadow_colour = "#4444AA",
                                  highlight_colour = "#ccaaff",
                                  background_colour = "#aaaaaa",
                                  foreground_colour = "#ffffff",
                               }),
                })

de.defstyle("frame-ionframe", {
                   based_on = "frame",
                   border_style = "inlaid",
                   padding_pixels = 1,
                   spacing = 0,
                   de.substyle("active", {
                                  highlight_pixels = 10,
                                  shadow_pixels = 10,
                                  padding_pixels = 10,
                                  border_style = "inlaid",
                                  shadow_colour = "#4422AA",
                                  padding_colour = "#CCAADD",
                                  highlight_colour = "#aa99CC",
                                  background_colour = "#aaaaaa",
                                  foreground_colour = "#ffffff",
                               }),
                })

de.defstyle("frame-floatframe", {
                   based_on = "frame",
                   border_style = "ridge"
                })

de.defstyle("tab", {
                   based_on = "*",
                   font = "-*-helvetica-medium-r-normal-*-12-*-*-*-*-*-*-*",
                   de.substyle("active-selected", {
                                  shadow_colour = "#8888aa",
                                  highlight_colour = "#333366",
                                  background_colour = "#666699",
                                  foreground_colour = "#eeeeee",
                               }),
                   de.substyle("active-unselected", {
                                  shadow_colour = "#777777",
                                  highlight_colour = "#cccccc",
                                  background_colour = "#aaa5aa",
                                  foreground_colour = "#000000",
                               }),
                   de.substyle("inactive-selected", {
                                  shadow_colour = "#aaaadd",
                                  highlight_colour = "#777788",
                                  background_colour = "#9999aa",
                                  foreground_colour = "#000000",
                               }),
                   de.substyle("inactive-unselected", {
                                  shadow_colour = "#777777",
                                  highlight_colour = "#cccccc",
                                  background_colour = "#bbbbbb",
                                  foreground_colour = "#222222",
                               }),
                   text_align = "center",
                })

de.defstyle("tab-frame", {
                   based_on = "tab",
                   de.substyle("*-*-*-*-activity", {
                                  shadow_colour = "#777777",
                                  highlight_colour = "#eeeeee",
                                  background_colour = "#990000",
                                  foreground_colour = "#eeeeee",
                               }),
                })

de.defstyle("tab-frame-ionframe", {
                   based_on = "tab-frame",
                   spacing = 0,
                })

de.defstyle("tab-frame-floatframe", {
                   based_on = "tab-frame",
                   de.substyle("active-selected", {
                                  shadow_colour = "#333366",
                                  highlight_colour = "#8888aa",
                                  background_colour = "#666699",
                                  foreground_colour = "#eeeeee",
                               }),
                   de.substyle("active-unselected", {
                                  shadow_colour = "#cccccc",
                                  highlight_colour = "#777777",
                                  background_colour = "#aaa5aa",
                                  foreground_colour = "#000000",
                               }),
                   de.substyle("inactive-selected", {
                                  shadow_colour = "#777788",
                                  highlight_colour = "#aaaadd",
                                  background_colour = "#9999aa",
                                  foreground_colour = "#000000",
                               }),
                   de.substyle("inactive-unselected", {
                                  shadow_colour = "#cccccc",
                                  highlight_colour = "#777777",
                                  background_colour = "#aaaaaa",
                                  foreground_colour = "#000000",
                               }),
                })

de.defstyle("tab-menuentry", {
                   based_on = "tab",
                   text_align = "left",
                   highlight_pixels = 1,
                   shadow_pixels = 1,
                })

de.defstyle("tab-menuentry-big", {
                   based_on = "tab-menuentry",
                   font = "-*-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*",
                   padding_pixels = 7,
                })

de.defstyle("input", {
                   based_on = "*",
                   font = "-Adobe-Courier-Medium-R-Normal--12-120-75-75-M-70-ISO8859-1",
                   shadow_colour = "#777777",
                   highlight_colour = "#eeeeee",
                   background_colour = "#aaaaaa",
                   foreground_colour = "#100040",
                   padding_pixels = 1,
                   highlight_pixels = 1,
                   shadow_pixels = 1,
                   border_style = "elevated",
                   de.substyle("*-cursor", {
                                  background_colour = "#300070",
                                  foreground_colour = "#aaaaaa",
                               }),
                   de.substyle("*-selection", {
                                  background_colour = "#8080aa",
                                  foreground_colour = "#300070",
                               }),
                })

gr.refresh()

