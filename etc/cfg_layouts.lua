--
-- Layouts for Ion
--

local a_frame = {
    type="WSplitRegion",
    regparams = {
        type = "WFrame", 
        frame_style = "frame-tiled"
    }
}


local horizontally_split = {
    -- Destroy workspace if the 'bottom' tiling is destroyed last
    bottom_last_close = true,
    -- Layout
    managed = {
        {
            type = "WTiling",
            bottom = true,
            -- The default is a single 1:1 horizontal split
            split_tree = {
                type = "WSplitSplit",
                dir = "horizontal",
                tls = 1,
                brs = 1,
                tl = a_frame,
                br = a_frame
            }
            -- For a single frame
            --split_tree = nil
        }
    }
}


local full_tiled = {
    -- Destroy workspace if the 'bottom' tiling is destroyed last
    bottom_last_close = true,
    -- Layout
    managed = {
        {
            type = "WTiling",
            bottom = true,
        }
    }
}


-- Let the world know about them

ioncore.deflayout("default", horizontally_split)
ioncore.deflayout("hsplit", horizontally_split)
ioncore.deflayout("full", full_tiled)
