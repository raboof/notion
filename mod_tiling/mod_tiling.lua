--
-- ion/mod_menu/mod_tiling.lua -- Tiling module stub loader
-- 
-- Copyright (c) Tuomo Valkonen 2004-2008.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- This is a slight abuse of the package.loaded variable perhaps, but
-- library-like packages should handle checking if they're loaded instead of
-- confusing the user with require/include differences.
if package.loaded["mod_tiling"] then return end

if not ioncore.load_module("mod_tiling") then
    return
end


-- Change default layout
if ioncore.getlayout("default")==ioncore.getlayout("empty") then
    ioncore.deflayout("default", {
        managed = {
            {
                type = "WTiling",
                bottom = true,
            }
        }
    })
end


-- Mark ourselves loaded.
package.loaded["mod_tiling"]=true
