de.defstyle("frame-tiled", {
    based_on = "frame",
    border_style = "inlaid",
    padding_pixels = 1,
    spacing = 1,
})

de.defstyle("frame-floating", {
    based_on = "frame",
    border_style = "ridge",
    bar = "shaped"
})

de.defstyle("frame-tiled-alt", {
    based_on = "frame-tiled",
    bar = "none",
})
