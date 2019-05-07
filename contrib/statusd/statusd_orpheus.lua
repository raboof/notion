-- Authors: Norbert Tretkowski <norbert@tretkowski.de>
-- License: Unknown
-- Last Changed: Unknown
--
-- Stolen from statusd_pytone.lua by Norbert Tretkowski <norbert@tretkowski.de>

if not statusd_orpheus then
  statusd_orpheus={
    interval=1*1000,
    infofile=os.getenv("HOME").."/.orpheus/currently_playing",
  }
end

local function get_orpheus_status()
  local f=io.open(statusd_orpheus.infofile,'r')
  if not f then return "n/a" end
  local playing=f:read()
  return playing
end

local orpheus_timer

local function update_orpheus()
    statusd.inform("orpheus", get_orpheus_status())
    orpheus_timer:set(statusd_orpheus.interval, update_orpheus)
end

-- Init
--get_inet_addr=get_inet_addr_fn()
orpheus_timer=statusd.create_timer()
update_orpheus()
