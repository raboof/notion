-- statusd_cpufreq.lua
--
-- Public domain
--
-- Use the key "cpufreq_[KMG]" to get the current CPU frequency in
-- K/M/GHz, according to /sys/devices/system/cpu/cpuX/cpufreq/.  (This
-- has the advantage of being a much "rounder" number than the one in
-- /proc/cpuinfo, as provided by statusd_cpuspeed.lua.)
-- 
-- The "cpu" option to the statusd settings for cpufreq modifies which
-- cpu we look at.

local defaults={ update_interval=2*1000, cpu=0 }
local settings=table.join(statusd.get_config("cpufreq"), defaults)

function get_cpufreq()
        local f=io.open('/sys/devices/system/cpu/cpu'.. settings.cpu ..'/cpufreq/scaling_cur_freq')
	local cpufreq_K = f:read('*a')
	f:close()
        
        local cpufreq_M = cpufreq_K / 1000
        local cpufreq_G = cpufreq_M / 1000

        return tostring(cpufreq_K), tostring(cpufreq_M), tostring(cpufreq_G)
end

function update_cpufreq()
	local cpufreq_K, cpufreq_M, cpufreq_G = get_cpufreq()
	statusd.inform("cpufreq_K", cpufreq_K)
	statusd.inform("cpufreq_M", cpufreq_M)
	statusd.inform("cpufreq_G", cpufreq_G)
	cpufreq_timer:set(settings.update_interval, update_cpufreq)
end

cpufreq_timer = statusd.create_timer()
update_cpufreq()
