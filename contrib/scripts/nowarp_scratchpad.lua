-- Authors: Etan Reisner <deryni@gmail.com>
-- License: MIT, see http://opensource.org/licenses/mit-license.php
-- Last Changed: 2007-01-23
--
--[[
Author: Etan Reisner
Email: deryni@gmail.com
Summary: Keeps ion from warping the pointer when activating a scratchpad region.
Version: 0.1
Last Updated: 2007-01-23

Copyright (c) Etan Reisner 2007

This software is released under the terms of the MIT license. For more
information, see http://opensource.org/licenses/mit-license.php .
--]]

local function do_warp_alt(reg)
	-- Make sure this is enough to always identify a scratchpad.
	if reg.mode and reg:mode() == "unknown" then
		return true
	end
	return false
end

local function setup_hooks()
	local hook

	hook = ioncore.get_hook("region_do_warp_alt")
	if hook then
		hook:add(do_warp_alt)
	end
end

setup_hooks()

-- vim: set expandtab sw=4:
