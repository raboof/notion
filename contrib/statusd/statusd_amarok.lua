-- Authors: Mark Tran <mark@nirv.net>
-- License: Unknown
-- Last Changed: Unknown
--
-- statusd_amarok.lua : Mark Tran <mark@nirv.net>

-- Display current track from Amarok

if not statusd_amarok then
   statusd_amarok = { interval = 3*1000 }
end

local function amarok_dcop()
   local f = io.popen('dcop amarok player nowPlaying 2> /dev/null', 'r')
   local amarok = f:read('*l')
   f:close()

   return amarok
end

local function amarok_status()
   local f = io.popen('dcop amarok player status 2> /dev/null', 'r')
   local status = f:read('*l')
   f:close()

   return status
end

local function change_display()
   local status = amarok_status()

   if status == "0" then
      return ""
   elseif status == "1" then
      local amarok = amarok_dcop()

      if (amarok ~= nil) then
	 return amarok.." (paused)"
      end
   elseif status == "2" then
      return amarok_dcop()
   else
      return ""
   end
end

local amarok_timer

local function update_amarok()
   statusd.inform("amarok", change_display())
   amarok_timer:set(statusd_amarok.interval, update_amarok)
end

amarok_timer = statusd.create_timer()
update_amarok()
