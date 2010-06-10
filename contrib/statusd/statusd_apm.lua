-- Adds capability for OpenBSD APM info in ion3 statusbar.
--
-- Originally written by Greg Steuck and released into the Public Domain
-- 2006-10-28 modified by Darrin Chandler <dwchandler@stilyagin.com>
-- 	to work with OpenBSD 4.0 apm output.
--
-- To install:
--   Save this file as ~/.ion3/statusd_apm.lua,
--   Change ~/.ion3/cfg_statusbar.lua like so:
-- 	Add "apm={}," to mod_statusbar.launch_statusd,
-- 	Modify "template" to include %apm_ variables
-- 	  e.g. template="[ %date || load:% %>load || bat: %apm_pct%%, A/C %apm_ac ],"
-- 
-- Available variables:
-- 	%apm_state	high, low, critical
-- 	%apm_pct	battery life (in percent)
-- 	%apm_estimate	battery life (in minutes)
-- 	%apm_ac		External A/C charger state
-- 	%apm_mode	Adjustment mode (manual, auto, cool running)
-- 	%apm_speed	CPU speed

local unknown = "?", "?", "?", "?", "?", "?"

function get_apm()
	local f=io.popen('/usr/sbin/apm', 'r')
	if not f then
		return unknown
	end
	local s=f:read('*all')
	f:close()
	local _, _, state, pct, estimate, ac, mode, speed =
		string.find(s,	"Battery state: (%S+), "..
				"(%d+)%% remaining, "..
				"([0-9]*) minutes life estimate\n"..
				"A/C adapter state: ([^\n]*)\n"..
				"Performance adjustment mode:%s(.+)%s"..
				"%((.+)%)\n"..
				".*"
				)
	if not state then
		return unknown
	end
	return state, pct, estimate, ac, mode, speed
end

local function inform(key, value)
	statusd.inform("apm_"..key, value)
end

local apm_timer

local function update_apm()
	local state, pct, estimate, ac, mode, speed = get_apm()
	local hint
	if statusd ~= nil then
		inform("state", state)
		inform("pct", pct)
		inform("estimate", estimate)
		if state == "high" then
			hint = "normal"
		elseif state == "low" then
			hint = "important"
		else
			hint = "critical"
		end
		if hint ~= nil then
			inform("state_hint", hint)
			inform("pct_hint", hint)
			inform("estimate_hint", hint)
		end
		inform("ac", ac)
		if ac == "connected" then
			hint = "important"
		else
			hint = "critical"
		end
		inform("ac_hint", hint)
		inform("mode", mode)
		inform("speed", speed)
		apm_timer:set(30*1000, update_apm)
	else
		io.stdout:write("Batt: "..pct.."% ("..state..
				", "..estimate.." mins)\n"..
				"A/C: "..ac.."\n"..
				"Mode: "..mode.."\n"..
				"CPU Speed: "..speed.."\n"
				)
	end
end

if statusd ~= nil then
	apm_timer = statusd.create_timer()
end
update_apm()
