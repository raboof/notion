-- Authors: Mario García H. <drosophila@nmental.com>
-- License: GPL, version 2
-- Last Changed: 2006-12-10
--
------------------------------------------------------------------------------------
--	
--	DESCRIPTION:
--	[ Multi Purpose Monitor for Ion3]
--	It detects if security logs, mboxes, maildirs, some files, etcetera were
--	changed. If they were changed in fact, shows a flashing (blinking) alarm with a
--	message text specified on settings (or default = !!).
--
--	If you specify your mail inbox it will do a flashing advise of new email.
--	If you specify a security log it will reflect your security warnings.
--	
--	You could specify whatever files or directories that do you want to monitor.
-->	All in Unix* like Oses is a file...
--
--	PLEASE READ THIS:
--    	  * This is another toy for Ion3.
--    	 ** It is not intended to replace or use Gamin, F.A.M. like libs.
--   	*** This is not a full whistles and sparkles power security monitor like Tripwire.
--     **** It is not a good idea to rely on this script to take care of sensible information.
--    ***** The level of recursion is only of one level for directories.
--   	    It only provides a very simple way to know if:
--
--  		-> Inode number was modified  or
--		-> Size has changed  or
--		-> User-group has changed  or
--		-> Access permisions have changed  or
--		-> Name changes (including file deletion and moving) then
--  		___________________________________________________________
--  		It flashes a specified message for 'certain specified time'
--	
--	PUPOSE:
--	To share a quick method for timers control and blinking patterns on Ion3 statusbars,
--	I mean, to fill statusbar(s) with annoying moving things. So don't use it ;]
--
-------	USAGE EXAMPLE : ------------------------------------------------------------
--	
--	If you don't know how to get this working please refer to Ion manual pages,
--	Ion home page or some other scripts. Then, write something like this on cfg_statusd.lua :
--	
--	mod_statusbar.create{
--		template = "whatever..  %flashing   ..whatever",   --> Modify this part.
--	},
--	
---->	And add somethig like this to:
--
--	mod_statusbar.launch_statusd{		
--		
--		flashing = {
--		files = {"/mnt/Feed_My_Dog", "~/Mail"} --> The intended purpose files (logs, mail)
--		log = ".ion3/flashing.log"	  	--> Some file in your $HOME[...] path.
--							    NOTE: $HOME is assumed by 'log'.
--							    Paths not in $HOME are invalid.
--		update_interval = 3000,		  	--> Time in milliseconds to update info.
--		flash_interval = 300, 		  	--> Speed of flashing pattern. (msecs.)
--			alarm_message = "!!",	  	--> Flashing Message (defaults are a good bet).
--			normal_message = "--",	  	--> Normal status message.
--			turn_off = 60,		  	--> This, avoids to show the annoying
--			  			    	    message for ever. The value represents
--		},				 	    cycles (10 * flash_interval msecs.)
--	}			
------------------------------------------------------------------------------------
--	
--	LICENSE: GPL2 Copyright(C)2006 Mario García H.
--	(See http://www.gnu.org/licenses/gpl.html to read complete license)
--
--	T.STAMP: Sun Dec 10 02:08:39 2006
--
--	DEPENDS: None at all.
--
--	INSECTS: You are the entomologist. You tell me.
--
-- 	NOTES ON USAGE:
--      - This script creates his own log of activity. You can choose a name and path on settings.
--      - If you remove the log, alarms will cease (rm -f *.log). Is not necessary to restart Ion.
--      - If you change the settings, the log will be auto-removed and re-written to reflect the changes
--        without false alarms.
--      - If, for some impossible circumstance, the status of some file or directory is normal again,
--        flashing will cease the annoying flashing thing by him self.
--      - If alarms are activated on certain time and you exit Ion current session, the
--        next session in Ion you will see the annoying thing on your screen... again.
--      - If you exit Ion and then you do changes to a file, the next time on Ion the alarms will do blinking.
--      - The minimum number for flashing interval (blink) is 300.
--	
--	CONTACT:
--	G.H. < drosophila (at) Nmental (dot) com>
--
------------------------------------------------------------------------------------

local defaults = {
	files = { "~/.ion3" },       --> If you like so much Ion, this is the better default.
				     --  If you change this setting, the log will be auto-updated!
	log = ".ion3/flashing.log",  --> Where do you like to log? Paths not in $HOME [...]
				     --  are invalid. If you change this file, please, remove
				     --	 your self the last file used. You are warned.
				     --	 NOTE: $HOME is assumed by 'log'.
	alarm_message = "!!",	     --> Put here the alarm message.
	normal_message = "--",	     --> Put here the normal status message.

	update_interval = 5*1000,    --> Check your files every X milliseconds.
	flash_interval = 400,        --> Blinking interval in milliseconds.
	turn_off = 500,		     --> Turn Off the alarm if it annoys you too much time:
				     --  (turn_off*flash_interval) milliseconds.
				     --> If you want permanent alarms: 3*999*999 is OK.
}
local settings = table.join(statusd.get_config("flashing"), defaults)

---SOME INSANE LOCALS :-------------------------------------------------------------

local flashing_timer
local is_ok = true
local flash = false
local home = os.getenv("HOME")
local message_lenght = string.len(settings.alarm_message)
local turn_off = settings.turn_off
settings.flash_interval = settings.flash_interval > 299 and settings.flash_interval or 300

---CONFIRM SOME INFO :--------------------------------------------------------------

local function confirm_someinfo()
	local keys = table.concat(settings.files, " * ")
	local log_file = home.."/"..settings.log
	
	local check_status = function()
		local check = io.popen("ls -liL --color=none "..table.concat(settings.files, " "))
		local status = check:read("*a"); check:close()
		return status
	end

	local new_log = function()
		local new = io.open(log_file, "w")
		if not new then
			return false
		else
			new:write(keys.."\n"..check_status())
			new:close()
			os.execute("chmod 400 "..log_file)
			new = io.open(log_file, "r")
			local log = new:read("*a"); new:close()
			return log
		end
	end
	
	local check_changes = function()
		local file = io.open(log_file, "r")
		if not file then
			return
		else
			local changes = file:read("*l"); file:close()
			return changes ~= keys and
			os.execute("rm -f "..log_file) or 1
		end
	end
	
	if not turn_off then
		turn_off = settings.turn_off 	--> Reinitialize turn_off if flash was done
		is_ok = true
		os.execute("rm -f "..log_file)
	end
	
	check_changes()	
	
	local file = io.open(log_file, "r")
	local log = file == nil and new_log() or file:read("*a")
	is_ok = keys.."\n"..check_status() == log
	return
end


---FLASHING PATERNS :---------------------------------------------------------------

local function flash_alarm()
	turn_off = turn_off > 0 and turn_off - 1 or false  --> Regresive shut down counter.
							   --  this is an extra ;>
	local show = function()
		statusd.inform("flashing", settings.alarm_message)
		statusd.inform("flashing_hint", "critical")
		return true
	end
	local hide = function()
		statusd.inform("flashing", string.rep(".", message_lenght))
		return false
	end
	flash = flash == false and show() or hide()  --> Do the real hide and show (flash)
	return
end


---FUNCTIONS CALLINGS :-------------------------------------------------------------

local function update_timer()
	confirm_someinfo() 		    --> A function checking something ... and changing between states
	if is_ok then			
		statusd.inform("flashing", settings.normal_message)
		statusd.inform("flashing_hint", "normal")
		flashing_timer:set(settings.update_interval, update_timer) --> Take different update_interval
	else
		flashing_timer:set(settings.flash_interval, update_timer)  --> Take different update_interval
		flash_alarm()
	end	
end
------------------------------------------------------------------------------------

flashing_timer = statusd.create_timer()
update_timer()

