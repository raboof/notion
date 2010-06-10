-- statusd_volume2.lua
-- Volume level and state information script
-- Written by Randall Wald
-- email: randy@rwald.com
-- Released under the GPL
-- 
-- Based on a public domain script written by Benjamin Sigonneau
-- 
-- This script uses "amixer" to find volume information. If you don't have
-- "amixer," this script will fail. Sorry.
-- Though this is labeled "statusd_volume2.lua", rename it to
-- "statusd_volume.lua" to make it work.
--
-- Available monitors:
-- 	%volume_level	Volume level, as a percentage from 0% to 100%
-- 	%volume_state	The string "" if unmuted, "MUTE " if muted
--
-- Example use:
-- 	template="[ %date || <other stuff> || vol: %volume_level %volume state]"
-- 	(note space between monitors but lack of space after %volume_state)
-- This will print
-- 	[ <Date> || <other stuff> || vol: 54% ]
-- when unmuted but
-- 	[ <Date> || <other stuff> || vol: 54% MUTE ]
-- when muted.

local function get_volume()
   local f=io.popen('amixer','r')
   local s=f:read('*all')
   f:close()
   local _, _, master_level, master_state = string.find(s, "%[(%d*%%)%] %[(%a*)%]")
   local sound_state = ""
   if master_state == "off" then
      sound_state = "MUTE "
   end
  return master_level.."", sound_state..""
end

local function inform_volume(name, value)
   if statusd ~= nil then
      statusd.inform(name, value)
   else
      io.stdout:write(name..": "..value.."\n")
   end
end
local function inform_state(value)
   if statusd ~= nil then
      statusd.inform("volume_state", value)
   else
      io.stdout:write("volume_state"..value.."\n")
   end
end

if statusd ~= nil then
   volume_timer = statusd.create_timer()
end

local function update_volume()
   local master_level, sound_state = get_volume()
   inform_volume("volume_level", master_level)
   inform_volume("volume_state", sound_state)
   if statusd ~= nil then
      volume_timer:set(500, update_volume)
   end
end

update_volume()
