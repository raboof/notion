-- dock-draw.lua drawing engine configuration file for ion-devel-dock
-- $Header: /home/twp/cvsroot/twp/ion/ion-devel-dock/dock-draw.lua,v 1.2 2003/12/21 11:59:48 twp Exp $

de_define_style("dock", {
    
    -- The following are the usual ion drawing configuration commands
    based_on = "*",
    
    -- outline_style controls where the border is drawn:
    -- "none" no border is drawn
    -- "all"  a single border is drawn around the dock
    -- "each" a border is drawn around each dockapp
    outline_style = "all",
    
    -- tile_size controls the minimum tile size
    tile_size = { width = 64, height = 64 },

})
