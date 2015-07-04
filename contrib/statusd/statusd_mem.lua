-- Authors: Mario Garcia H. <drosophila@nmental.com>
-- License: GPL, version 2
-- Last Changed: 2006-12-07
--
-------------------------------------------------------------------------------------------
--
--	PURPOSE: 
--	Shows system available memory catching [free] command outputs.
--	It is intended to make it simpler than statusd_meminfo, plus user configurable 
--	measurement units and alarms for "all" available memory metters. 
--
--	USAGE:
--	Just set any of the following labels on cfg_statusbar.lua: %mem_hused, %mem_shared
--	%mem_free, %mem_hfree, %mem_swap, %mem_used, %mem_cached. Example: [MF: %mem_free]
--	
--	MEANINGS:
-->	** "mem_hfree" poses as "htop free memory" or "mem_free +cached +buffers",
--	in oposition, "mem_hused" is "mem_used -cached -buffers"; other labels have
--	transparent meanings.
--
------- CONFIG EXAMPLE:	------------------------------------------------------------------
--
--	To modify settings is quite simple and flexible, write (on cfg_statusbar.lua)
--	something like this, without comments:
--					mem = {
--					   update_interval = 15*1000, --> Milliseconds
--					   free_alarm = 25,   --> Limits percentaje ...
--					   used_alarm = 65,
--					   units = "m" 	      --> "g" or "k" too
--					   }
--	Write only the settings that do you want to change or leave this section as is...
-->	** "update_interval" means "time in milliseconds to update info (default = 15)"
--	"xx_alarm" means "do a color advise when memory *percentage* reaches this value".
--	(both defaults are 50). "units" means Gb "g", Mb "m" or Kb "k" (default = "m")
------------------------------------------------------------------------------------------
--
--	NOTES: 
--    *	Alarms for used memory are inverse to alarms for free memory (think about it...)
--	"mem_total" label is useless. If total memory varies, its time to open your
--	hardware and check this script from barebone. Seriously, may be your video or wifi
--	devices were claiming some free R.A.M. on your machine start-up. 
--	However, I included "mem_total" just in case.
--   ** This script has non blocking I/O.
--
--	LICENSE: 
--	GPL2 Copyright(C)2006 Mario Garcia H.
--	(Please see http://www.gnu.org/licenses/gpl.html to read complete license)
--
--	T.STAMP: Thu Dec  7 03:28:04 2006
--	
--	DEPENDS: "free" command. Probably, all GNU/Linux distros have one.
--	
--	INSECTS: Not known. 
--	
--	CONTACT:
--	G.H. <drosophila (at) nmental (dot) com>
--
------- DEFAULT SETTINGS :-----------------------------------------------------------------

local mem_timer
local defaults = { update_interval = 15*1000, free_alarm = 50, used_alarm = 50, units = "m" }
local settings = table.join(statusd.get_config("mem"), defaults)
local free_version_major
local free_version_minor

------- MEM MONITOR :----------------------------------------------------------------------

local function show_meminfo(status)
	while status do	
		local ok, _, total, used, free, shared, buffers, cached =--
		string.find(status, "Mem:%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)")
		--
		if not ok then statusd.inform("mem_template", "--") return end
		--
		statusd.inform("mem_total", total)
		statusd.inform("mem_used", used)
		statusd.inform("mem_free", free)
		statusd.inform("mem_shared", shared)
		statusd.inform("mem_buffers", buffers)
		--
		if (3.3 <= free_version_major) and (10 <= free_version_minor) then
		  local avail = cached
		  -- In 3.3.10 and later, there is no separate cached column. Instead it is
		  -- the total amount available. So...

		  -- Use the buffers amount for cached
		  statusd.inform("mem_cached", buffers)
		  -- Total used is now total - available
		  statusd.inform("mem_hused", tostring(total - avail))
		  -- Total free is the amount available
		  statusd.inform("mem_hfree", avail)
		else
		  statusd.inform("mem_cached", cached)
		  statusd.inform("mem_hused", tostring(used - cached - buffers))
		  statusd.inform("mem_hfree", tostring(free + cached + buffers))
		end
		--
		statusd.inform("mem_used_hint",
		used*100/total >= settings.used_alarm and "critical" or "important")
		statusd.inform("mem_hused_hint",
		(used - cached - buffers)*100/total >= settings.used_alarm and "critical" or "important")
		statusd.inform("mem_free_hint",
		free*100/total <= settings.free_alarm and "critical" or "important")
		statusd.inform("mem_hfree_hint",
		(free + cached + buffers)*100/total <= settings.free_alarm and "critical" or "important")
		--
		status = coroutine.yield()
	end
end

local function update_mem()
	if (3.3 <= free_version_major) and (10 <= free_version_minor) then
	  statusd.popen_bgread("free -"..settings.units, coroutine.wrap(show_meminfo))
	else
	  statusd.popen_bgread("free -"..settings.units.."o", coroutine.wrap(show_meminfo))
	end
	mem_timer:set(settings.update_interval, update_mem)
end

local function get_free_version()
      local ok, _, version_info
      
      -- Run free with the -V option to get its version information
      fh = io.popen("free -V", "r")
      version_info = fh:read()
      fh.close()
      
      ok, _, free_version_major, free_version_minor =--
        string.find(version_info, "(%d+%.%d+)%.(%d+)")

      if not ok then statusd.inform("mem_template", "--") return end

      free_version_major = tonumber(free_version_major)
      free_version_minor = tonumber(free_version_minor)
end

get_free_version()
mem_timer = statusd.create_timer()
update_mem()

