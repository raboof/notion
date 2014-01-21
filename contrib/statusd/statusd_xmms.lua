-- Authors: Peter Randeu <ranpet@sbox.tugraz.at>
-- License: GPL, version 2
-- Last Changed: Unknown
--
-- statusd_xmms.lua
--
-- Gets title of song currently selected in xmms' playlist.
-- Depends on xmms and pyxmms-remote
-- Inspired by statusd_mpd.lua
-- Written by Peter Randeu < ranpet at sbox dot tugraz dot at >
--
-- You are free to distribute this software under the terms of the GNU
-- General Public License Version 2.

if not statusd_xmms then
  statusd_xmms={
      interval=10*1000,
  }
end

local settings = table.join (statusd.get_config("xmms"), statusd_xmms)

local function get_xmms_status()
      local f = io.popen('pyxmms-remote get_playlist_pos', 'r')
      local pl_num = f:read()
      if not pl_num then
          return "pyxmms-remote not available"
      end
      f:close()
      f = io.popen('pyxmms-remote get_playlist_title ' .. pl_num, 'r')
      title = f:read()
      f:close()
      return title
end

local xmms_timer

local function update_xmms()
      statusd.inform("xmms", get_xmms_status())
      xmms_timer:set(settings.interval, update_xmms)
end

xmms_timer = statusd.create_timer()
update_xmms()
