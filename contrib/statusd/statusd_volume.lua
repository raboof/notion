-- Authors: Benjamin Sigonneau
-- License: Public domain
-- Last Changed: Unknown
--
-- Public domain, written by Benjamin Sigonneau
-- Allows displaying volume information in the statusbar.
--
-- add some of the following fields into your template in cfg_statusbar.lua:
--     %volume_master
--     %volume_pcm

local unknown = "??", "??"

local function get_volume()
   local f=io.popen('aumix -q','r')
   local s=f:read('*all')
   f:close()
   local _, _, master, pcm =
      string.find(s, "vol[0-9]? (%d*), .*\n"..
                     "pcm[0-9]? (%d*), .*\n"
               )

   if not master then
      return unknow
   elseif not pcm then
      return unknow
   end

  return master.."%", pcm.."%"
end


local function inform(key, value)
   statusd.inform("volume_"..key, value)
end


local volume_timer = statusd.create_timer()

local function update_volume()
   local master, pcm = get_volume()
   inform("master", master)
   inform("pcm", pcm)
   -- update every 10 seconds
   volume_timer:set(10*1000, update_volume)
end

update_volume()

