-- Authors: Etan Reisner <deryni@gmail.com>
-- License: MIT, see http://opensource.org/licenses/mit-license.php
-- Last Changed: 2007-01-29
--
--[[
Author: Etan Reisner
Email: deryni@gmail.com
Summary: Toggle (and create) floating WGroupWS:s by name.
Version: 0.1
Last Updated: 2007-01-29

Copyright (c) Etan Reisner 2007

This software is released under the terms of the MIT license. For more
information, see http://opensource.org/licenses/mit-license.php .
--]]

-- Example: This will create a level 2 floatws named example_floatws
--          kpress(MOD4.."e", "named_groupws(_, 'example_groupws')")

function named_groupws(reg, name)
    local named_groupws
    local scr = reg:screen_of()

    named_groupws = ioncore.lookup_region(name, "WGroupWS")

    if not named_groupws then
        named_groupws = scr:attach_new({
                                        type="WGroupWS",
                                        name=name,
                                        unnumbered=true,
                                        hidden=true,
                                       })
    end

    scr:set_hidden(named_groupws, 'toggle')
end

-- vim: set expandtab sw=4:
