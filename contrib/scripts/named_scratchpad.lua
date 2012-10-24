--[[
Author: Etan Reisner
Email: deryni@gmail.com
Summary: Toggle (and create) scratchpads by name.
Version: 0.2
Last Updated: 2007-01-23

Copyright (c) Etan Reisner 2007

This software is released under the terms of the MIT license. For more
information, see http://opensource.org/licenses/mit-license.php .
--]]

-- Usage: This will create a scratchpad named example_sp
--          kpress(MOD4.."space", "named_scratchpad(_, 'example_sp')")

function named_scratchpad(reg, name)
    local named_sp
    local default_w, default_h = 640, 480
    local scr = reg:screen_of()
    local geom_scr = scr:geom()

    local geom_loc = {
        w = math.min(geom_scr.w, default_w),
        h = math.min(geom_scr.h, default_h),
    }
    geom_loc.x = (geom_scr.w - geom_loc.w) / 2
    geom_loc.y = (geom_scr.h - geom_loc.h) / 2

    named_sp = ioncore.lookup_region(name, "WFrame")

    if not named_sp then
        named_sp = scr:attach_new({
                                   type="WFrame",
                                   name=name,
                                   unnumbered=true,
                                   modal=true,
                                   hidden=true,
                                   sizepolicy=5,
                                   geom=geom_loc,
                                  })
    end

    mod_sp.set_shown(named_sp, "toggle")
end

-- vim: set expandtab sw=4:
