-- dock-draw.lua drawing engine configuration file for ion-devel-dock
-- $Header: /home/twp/cvsroot/twp/ion/ion-devel-dock/dock-draw.lua,v 1.1 2003/12/02 18:36:39 twp Exp $

-- This configuration is designed to look good with look-clean.lua, modify it
-- to suit your tastes.

de_define_style("dock", {
	
	-- The following are the usual ion drawing configuration commands
    based_on = "*",
    bar_inside_frame = true,
    highlight_colour = "white",
    shadow_colour = "white",
    background_colour = "#8a999e",
    foreground_colour = "white",
    spacing = 1,
    shadow_pixels = 1,
    highlight_pixels = 1,
    padding_pixels = 0,
	
	-- outline_style controls where the border is drawn:
	-- "all" a single border should be drawn around all the dockapps
	-- "each" a border is drawn around each dockapp
    outline_style = "all",
	
	-- tile_size controls the minimum tile size
    tile_size = {width=64, height=64},

})
