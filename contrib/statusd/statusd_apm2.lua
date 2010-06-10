-- Public domain, written by Greg Steuck
-- Edited and updated by Gerald Young -- ion3script@gwy.org
-- This works on FreeBSD's apm (5.x) program added some color to indicate 
-- AC connection and status (Charging, low, critical)
-- Allows displaying apm information in the statusbar.
-- To install:
--    save this file into ~/.ion3/statusd_apm.lua,
--    copy the default cfg_statusbar.lua to ~/.ion3, edit it to include (~line 81):
--    -- Battery meter
--    --[[
--    apm={},
--    --]]
--       add some of the following fields into your template in cfg_statusbar.lua:
--       %apm_ac: A/C cable on-line (connected) off-line (disconnected)
--       %apm_pct: percent of remaining battery
--       %apm_estimate: time remaining based upon usage ... or whatever apm thinks.
--       %apm_state: Status: charging/high/low/critical
-- in cfg_statusbar.lua, about line 28, add the next line without the leading "--"
-- template="[ %date || load:% %>load_1min || battery: %apm_pct AC: %apm_ac Status: %apm_state ]",
-- If you've already customized your template= line, then simply add the field(s) where you want.

local unknown = "?", "?", "?", "?"

-- Runs apm utility and grabs relevant pieces from its output.
-- Most likely will only work on OpenBSD due to reliance on its output pattern.
function get_apm()
    local f=io.popen('/usr/sbin/apm', 'r')
    if not f then
        return unknown
    end
    local s=f:read('*all')
    f:close()
    local _, _, ac, state, pct, estimate = 
	string.find(s, 
                      "AC Line status: (.*)\n"..
		      "Battery Status: (.*)\n"..
		      "Remaining battery life: (.*)\n"..
		      "Remaining battery time: (.*)\n"
		       )
    if not state then
	return unknown
    end
    return state, pct, estimate, ac
end

local function inform(key, value)
    statusd.inform("apm_"..key, value)
end

local apm_timer = statusd.create_timer()

local function update_apm()
    local state, pct, estimate, ac = get_apm()
    local stateinf
    if state=="low" then
	stateinf = "important" 
    end
    if state == "critical" then
        stateinf = "critical"
    end
    if state == "charging" then
        stateinf = "important"
    end
    inform("state", state)
    inform("state_hint", stateinf)
    inform("pct", pct)
    inform("estimate", estimate)
    if ac == "off-line" then 
	stateinf="critical"
    end
    if ac == "on-line" then 
        stateinf="important"
    end
    inform("ac", ac)
    inform("ac_hint", stateinf)
    apm_timer:set(60*1000, update_apm)
end

update_apm()

