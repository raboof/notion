-- Authors: Alexander Wirt <formorer@formorer.de>
-- License: Unknown
-- Last Changed: Unknown
--
-- Stolen from statusd_mpd.lua
-- Responsible for bugs and so on: Alexander Wirt <formorer@formorer.de>
-- To get this working you have to enable playerinfofile in your pytonerc
-- If you have the info file at an unusal place overwrite the infofile setting

if not statusd_pytone then
  statusd_pytone={
    interval=1*1000,
    infofile=os.getenv("HOME").."/.pytone/playerinfo",
  }
end




local function get_pytone_status()
  local f=io.open(statusd_pytone.infofile,'r')
  if not f then return "n/a" end
  local playing=f:read()
  return playing 
end

local pytone_timer

local function update_pytone()
    statusd.inform("pytone", get_pytone_status())
    pytone_timer:set(statusd_pytone.interval, update_pytone)
end

-- Init
--get_inet_addr=get_inet_addr_fn()
pytone_timer=statusd.create_timer()
update_pytone()
