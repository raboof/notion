-- Authors: James Gray <james@solitaryfield.org>, Etan Reisner <deryni@gmail.com>
-- License: Unknown
-- Last Changed: 2007-06-22
--
--[[
Author: James Gray, Etan Reisner
Email: james at solitaryfield dot org, deryni@unreliablesource.net
IRC: yaarg, deryni
Summary: Allows locking
Version: 0.2
Last Updated: Fri Jun 22 00:59:06 EDT 2007
--]]

-- Usage:
-- F11 toggles the lock of keyboard close and resize on the currently active
-- frame.
-- META-F11 will save the name of the frame and lock it again in subsequent
-- sessions of ion.
--
-- You will need to rebind your close and kill bindings to use the
-- check_before_close and check_before_kill functions for this script to do
-- anything.
--
-- Adding
--    de.substyle("*-*-*-locked", {
--        background_colour = "yellow",
--    }),
--
--    de.substyle("*-*-*-*-locked_saved", {
--        background_colour = "white",
--    }),
-- to your theme's de.defstyle("tab-frame", {...}) style will cause locked and
-- locked+saved frames to be highlighted.

local locks = setmetatable({}, {__mode="k"})
local saved = {}

function lock_frame(frame, save)
	local name = frame:name()

	if locks[frame] then
		locks[frame] = nil
		frame:set_grattr("locked", "unset")
		if save and name then
			frame:set_grattr("locked_saved", "unset")
			saved[name] = nil
		end
	else
		locks[frame] = true
		frame:set_grattr("locked", "set")
		if save and name then
			frame:set_grattr("locked_saved", "set")
			saved[name] = true
		end
	end
end

function check_before_kill(reg)
	if not locks[reg] then
		WClientWin.kill(reg)
	end
end

function check_before_close(reg, sub)
	if (not locks[reg]) and (not locks[sub]) then
		WRegion.rqclose_propagate(reg, sub)
	end
end

ioncore.defbindings("WFrame",{
	kpress("F11", "lock_frame(_)"),
	kpress(META.."F11", "lock_frame(_, true)")
})

function save_locked()
	ioncore.write_savefile("saved_lock_frame", saved)
end

function load_locked()
	local locked = ioncore.read_savefile("saved_lock_frame") or {}

	for k,v in pairs(locked) do
		local reg = ioncore.lookup_region(k)
		lock_frame(reg, true)
	end
end

local hook = ioncore.get_hook("ioncore_deinit_hook")
if hook then
	hook:add(save_locked)
end
hook = nil

load_locked()
