-- Authors: Sadrul Habib Chowdhury <imadil@gmail.com>
-- License: Public domain
-- Last Changed: Unknown
--
-- statusd_netmon.lua: monitor the speed of a network interface
--
-- Thanx to Tuomo for pointing out a problem in the previous script.
-- 
-- In case this doesn't work for someone, do let me know :)
--
-- Author
-- Sadrul Habib Chowdhury (Adil)
-- imadil at gmail dot com
--
-- Support for per-stat monitors and thresholds added by Jeremy
-- Hankins.
--
--
-- Monitor values available with this monitor:
--
-- netmon
-- netmon_kbsin
-- netmon_kbsout
-- netmon_avgin
-- netmon_avgout
-- netmon_count
--
-- To use the average values or the count you need show_avg and
-- show_count turned on, respectively.  If you want the default format
-- (which you get with %netmon) but with colors for important and
-- critical thresholds, try:
--
-- %netmon_kbsin/%netmon_kbsout (%netmon_avgin/%netmon_avgout)

if not statusd_netmon then
  statusd_netmon = {
      device = "eth0",
      show_avg = 1,       -- show average stat?
      avg_sec = 60,       -- default, shows average of 1 minute
      show_count = 0,     -- show tcp connection count?
      interval = 1*1000,  -- update every second

      -- Threshold information.  These values should likely be tweaked to
      -- suit local conditions.
      important = {
	kbsin = 1/10,
	kbsout = 1/10,
	avgin = 1/10,
	avgout = 1/10,
	count = 4,
      },

      critical = {
 	kbsin = 30,
	kbsout = 30,
	avgin = 5,
	avgout = 5,
	count = 50,
    }
}
end

local timer = nil       -- the timer
local positions = {}    -- positions where the entries will be
local last = {}         -- the last readings
local history_in = {}   -- history to calculate the average
local history_out = {}
local total_in, total_out = 0, 0
local counter = 0       --

local settings = table.join(statusd.get_config("netmon"), statusd_netmon)

--
-- tokenize the string
--
local function tokenize(str)
    local ret = {}
    local i = 0
    local k = nil

    for k in string.gfind(str, '(%w+)') do
        ret[i] = k
        i = i + 1
    end
    return ret
end

-- 
-- get the connection count
--
local function get_connection_count()
    local f = io.popen('netstat -st', 'r')
    if not f then return nil end

    local output = f:read('*a')
    if string.len(output) == 0 then return nil end

    local s, e, connections =
	string.find(output, '%s+(%d+)%s+connections established%s')
    f:close()
    return tonumber(connections)
end

--
-- calculate the average
--
local function calc_avg(lin, lout)
    if counter == settings.avg_sec then
        counter = 0
    end

    total_in = total_in - history_in[counter] + lin
    history_in[counter] = lin

    total_out = total_out - history_out[counter] + lout
    history_out[counter] = lout

    counter = counter + 1

    return total_in/settings.avg_sec, total_out/settings.avg_sec
end

--
-- parse the information
--
local function parse_netmon_info()
    local s
    local lin, lout

    for s in io.lines('/proc/net/dev') do
        local f = string.find(s, settings.device)
        if f then
            local t = tokenize(s)
            return t[positions[0]], t[positions[1]]
       end
    end
    return nil, nil
end

--
-- Return a hint value for the given meter
--
local function get_hint(meter, val)
    local hint = "normal"
    local crit = settings.critical[meter]
    local imp = settings.important[meter]
    if crit and val > crit then
	hint = "critical"
    elseif imp and val > imp then
	hint = "important"
    end
    return hint
end

--
-- update the netmon monitor
--
local function update_netmon_info()
    local s
    local lin, lout

    local function fmt(num)
	return(string.format("%.1fK", num))
    end

    lin, lout = parse_netmon_info()
    if not lin or not lout then
	-- you should never reach here
        statusd.inform("netmon", "oops")
	statusd.inform("netmon_hint", "critical")
        return
    end

    last[0], lin = lin, lin - last[0]
    last[1], lout = lout, lout - last[1]

    local kbsin = lin/1024
    local kbsout = lout/1024

    local output = string.format("%.1fK/%.1fK", kbsin, kbsout)

    if settings.show_avg == 1 then
	local avgin, avgout = calc_avg(lin/1024, lout/1024)
        output = output .. string.format(" (%.1fK/%.1fK)", avgin, avgout)

	statusd.inform("netmon_avgin", fmt(avgin))
	statusd.inform("netmon_avgin_hint",
		       get_hint("avgin", avgin))
	statusd.inform("netmon_avgout", fmt(avgout))
	statusd.inform("netmon_avgout_hint",
		       get_hint("avgout", avgout))
    end
    if settings.show_count == 1 then
	local count = get_connection_count()
	if count then
	    output = "[" .. tostring(count) .. "] " .. output

	    statusd.inform("netmon_count", tostring(count))
	    statusd.inform("netmon_count_hint",
			   get_hint("count", count))
	else
	    output = "[?] " .. output
	    statusd.inform("netmon_count", "?")
	    statusd.inform("netmon_count_hint", "critical")
	end
    end

    statusd.inform("netmon_kbsin", fmt(kbsin))
    statusd.inform("netmon_kbsin_hint",
		   get_hint("kbsin", kbsin))
    statusd.inform("netmon_kbsout", fmt(kbsout))
    statusd.inform("netmon_kbsout_hint",
		   get_hint("kbsout", kbsout))
    statusd.inform("netmon", output)

    timer:set(settings.interval, update_netmon_info)
end

--
-- is everything ok to begin with?
--
local function sanity_check()
    local f = io.open('/proc/net/dev', 'r')
    local e

    if not f then
        return false
    end

    local s = f:read('*line')
    s = f:read('*line')         -- the second line, which should give
                                -- us the positions of the info we seek

    local t = tokenize(s)
    local n = #(t)
    local i = 0

    for i = 0,n do
        if t[i] == "bytes" then
            positions[0] = i
            break
        end
    end

    i = positions[0] + 1

    for i=i,n do
        if t[i] == "bytes" then
            positions[1] = i
            break
        end
    end

    if not positions[0] or not positions[1] then
        return false
    end

    s = f:read('*a')        -- read the whole file
    if not string.find(s, settings.device) then
        return false        -- the device does not exist
    end
    
    return true
end

--
-- start the timer
-- 
local function init_netmon_monitor()
    if sanity_check() then
        timer = statusd.create_timer()
        last[0], last[1] = parse_netmon_info()
        
        if settings.show_avg == 1 then
            for i=0,settings.avg_sec-1 do
                history_in[i], history_out[i] = 0, 0
            end
        end
        
        statusd.inform("netmon_template", "xxxxxxxxxxxxxxxxxxxxxxx")
        update_netmon_info()
    else
        statusd.inform("netmon", "oops")
        statusd.inform("netmon_hint", "critical")
    end
end

init_netmon_monitor()

