--------------------------------------------------------------------------------------------
-- 	
-- 	PURPOSE: 
-- 	Flexible Maildir monitor with configurable alarms and optional launcher for external
-- 	commands. The name "nmaild" stands for "aNy-mail-directories".
--
--------------------------------------------------------------------------------------------
--> 	  Meters: %nmaild_new, %nmaild_read, %nmaild_allnew and  %nmaild_allread 	 <--
--------------------------------------------------------------------------------------------	
--
--	If you do not customize default settings, this script detects the environment
--	variable "$MAILDIR". If that variable is not set, then ~/Maildir will be used as
--	default path. Otherwise, %nmaild_new and  %nmaild_read are counters for new and
--	read emails into the first maildir found in settings. %nmaild_allnew and
--	%nmaild_allread are counters of new and read emails into the other mail directories
--	specified.
--
--	(Example: ~/Maildir/home, ~/Maildir/work, ~/Maildir/forum_lists, ~/Mail/FSCK,
--	spam, etc. where ~Maild/home is the main directory, others are counted as all*) 
--
--	USAGE:	
--    * If the directory ~/.ion3 does not exist create it and copy cfg_statusbar.lua
--      (the file is located somewhere in your /usr/share/ion3 or /usr/local/share/ion3
--      or /etc/X11/ion3 depending on ion3 installation). We need that file located on
--      our $HOME to edit and override Ion3 system configurations.
--
--   ** The file should look like this:
--     	template = "[Mail %nmaild_new:%nmaild_all|%nmaild_allnew%nmaild_allread]"
--     	
--     	After you restart ion3 (killall -USR1 ion3) or (session/restart on Ion menu),
--     	the statusbar will look like this:
--	
--	[Mail: 1:1|4:23] where |1:1| is meaning one mail new and one mail read in first dir.
--
--	You can change this script behavior by writting something like this in
--	cfg_statusbar.lua:
--
-----> 	CONFIGURATION EXAMPLE: ------------------------------------------------------------
--
--    	nmaild = {		           Values not in config will be assumed from defaults
--	    update_interval = 15*1000, --> Miliseconds (default is 15 seconds)
--	    check = {		       --> Put here the list of maildirs to 'check' for.
--		    "~/Maildir",           Please, pay attention to the logical structure.
--		    "~/Maildir/work",
--		    "~/Maildir/copies",
--		    "~/Security/spam",
--		    "~/Maildir/lists",
--		    "/xmail/office/susan",
--		    "/Mail/common",
--		    },
--
--	    	     new = {"critical", 1},   -->  Alarms: For new, read, allnew and allread.
--	    	     read = {"important", 5},      --------------------------
--	   	     allnew = {"critical", 5},	   Syntax: mailfilter = {"hint", value},
--	    	     allread = {"important", 10},
--	    	     exec_on_new = "play ~/mew_wmail.mp3" -->  Execute something on new email arrival.
--	    	     					       If you want to deactivate exec_on_new,
--	    	     					       just erase it from settings.
--	    	     					       If you need to specify very complex commands
--	    	     					       the best way is to replace the quotes for
--	    	     					       [[ at start and ]] at the end.
--
--  	},				      -->  Take care, write correct config endings. 	   
--  						
--  						   Meanings:
--  						   --------------------------
--  						   "hint" means the color of alarm: If you are
--  						   daredevil, edit the file lookcommon_XXX.lua
--  						   (You must have a copy into ~/.ion3 directory)
--  						   to change colors or to add more  (xcolors).
--  						   If 'value' reaches (is >= than) the number
--		 				   you put here, alarms will be displayed !!. 
--
-------------------------------------------------------------------------------------------
--
--	Internal cycles are provided by string.gsub() so, hopefully, we avoid unnecesary
--	use of lines and variables. This script will do only four (4) callings, no matters
--	the number of maildirs specified in cfg_statusbar.
--
--	VERSION: 1.a
--	
--	LAST CHANGES:
--	Added "exec_on_new" function. You don't need to use it, but it does more fancy the
--	script. Now, it can do audio advices, launch your email client or show [g-k-x]message
--	when new emails are detected.
--
--	TO-DO:
--	To show the amounts of disk used. If someone finds interesting that addition...
--
-- 	LICENCE:
-- 	GPL2 Copyright (C) 2006 Mario García H.
-- 	See http://www.gnu.org/licenses/gpl.html to read complete licence)
--
-- 	T.STAMP: sat nov 18 01:03:50 COT 2006
--
-- 	CONTACT:
-- 	<drosophila (at) Nmental (dot) com>
--
------ 	DEFAULT CONFIGURATION : -----------------------------------------------------------

local nmaild_timer
local last_count = { new = 0, allnew = 0 }
local defaults = {
		update_interval = 5000,
		check = { os.getenv("MAILDIR") or "~/Maildir" },
		exec_on_new = false,		--> Use this to play sounds , to execute an
		new = {  "critical", 1 },	--  email client, etc. on new email arrival.
		read = { "important", 5 },
		allnew = { "critical", 5 },
		allread = {"important", 20 }
	}
local settings = table.join(statusd.get_config("nmaild"), defaults)

------ 	SCRIPT : --------------------------------------------------------------------------

local function exec_on_new()
	if settings.exec_on_new then
		os.execute(settings.exec_on_new.." 2>/dev/null &")
	end
	return
end

local get_count = function(nmaild, label)
	local read_dirs = io.popen("du -a " ..nmaild.. " 2>/dev/null", "r")
	local count = read_dirs:read("*a"); read_dirs:close()
	_, count = string.gsub(count, "%d+%.%C+%.%C+", "") --> Simple filter.
	
	local hint = count >= settings[label][2] and settings[label][1] or "normal"
	statusd.inform("nmaild_" ..label, tostring(count))
	statusd.inform("nmaild_" ..label.. "_hint", hint)
	
	if (label == "new" or label == "allnew") and (count > last_count[label]) then
		last_count[label] = count
		exec_on_new()
		return
	end
	if count == 0 and (label == "new" or label == "allnew") then 
		last_count[label] = 0
		return
	end
end

local function plan_count()
	get_count(settings.check[1] .."/new", "new"); coroutine.yield()
	get_count(settings.check[1] .."/cur", "read"); coroutine.yield()
	get_count(table.concat(settings.check, "/new ", 2) .."/new", "allnew"); coroutine.yield()
	get_count(table.concat(settings.check, "/cur ", 2) .."/cur", "allread")
	return
end

local function update_nmaild()
	local threads = coroutine.create(plan_count) --> Trying to avoid read bottlenecks  ,>
	while coroutine.resume(threads) do end	     --> Without threads the timer will die :(
	nmaild_timer:set(settings.update_interval, update_nmaild)
end

nmaild_timer =  statusd.create_timer()
update_nmaild()

