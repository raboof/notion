-- statusd_linuxbatt.lua
--
-- Public domain
--
-- Uses the /proc/acpi interface to get battery percentage.
--
-- Use the key "linuxbatt" to get the battery percentage; use
-- "linuxbatt_state" to get a symbol indicating charging "+",
-- discharging "-", or charged " ".
--
-- Now uses lua functions instead of bash, awk, dc.  MUCH faster!
--
-- The "bat" option to the statusd settings for linuxbatt modifies which
-- battery we look at.

local defaults={
        update_interval=15*1000,
        bat=0,
        important_threshold=30,
        critical_threshold=10,
}
local settings=table.join(statusd.get_config("linuxbatt"), defaults)

function linuxbatt_do_find_capacity()
        local f=io.open('/proc/acpi/battery/BAT'.. settings.bat ..'/info')
        local infofile=f:read('*a')
        f:close()
        local i, j, capacity = string.find(infofile, 'last full capacity:%s*(%d+) .*')
        return capacity
end

local capacity = linuxbatt_do_find_capacity()

function get_linuxbatt()
	
        local f=io.open('/proc/acpi/battery/BAT'.. settings.bat ..'/state')
	local statefile=f:read('*a')
	f:close()
        local i, j, remaining = string.find(statefile, 'remaining capacity:%s*(%d+) .*')
        local percent = math.floor( remaining * 100 / capacity )

        local i, j, statename = string.find(statefile, 'charging state:%s*(%a+).*')
        if statename == "charging" then
                return percent, "+"
        elseif statename == "discharging" then
                return percent, "-"
        else
                return percent, " "
        end
end

function update_linuxbatt()
	local perc, state = get_linuxbatt()
	statusd.inform("linuxbatt", tostring(perc))
	statusd.inform("linuxbatt_state", state)
        if perc < settings.critical_threshold
        then statusd.inform("linuxbatt_hint", "critical")
        elseif perc < settings.important_threshold
        then statusd.inform("linuxbatt_hint", "important")
        else statusd.inform("linuxbatt_hint", "normal")
        end
	linuxbatt_timer:set(settings.update_interval, update_linuxbatt)
end

linuxbatt_timer = statusd.create_timer()
update_linuxbatt()
