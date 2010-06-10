-- statusd_maildir.lua - Gets new and total counts of mails in a Maildir structure
-- Copyright (c) 2005 Brett Parker <iDunno@sommitrealweird.co.uk>
-- 
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.
-- 
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
-- 
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

-- This exports the variables %maildir_MAILDIRNAME, %maildir_MAILDIRNAME_new
-- and %maildir_MAILDIRNAME_total to the status bar where MAILDIRNAME is
-- the key in the maildirs setting.
--
-- The 2 settings available in the cfg_statusbar.lua are:
-- 	interval - this is the number of milliseconds between each check
-- 	maildirs - this is a key value list of Maildirs, the key is used
--                 for MAILDIRNAME above.
--
-- The defaults update every 10 seconds with a maildir of ~/Maildir/

if not statusd_maildir then
  statusd_maildir={
      interval=10000,
      maildirs = {INBOX="~/Maildir/"},
  }
end

local settings = table.join (statusd.get_config("maildir"), statusd_maildir)

local function get_num_files(directory)
	local f = io.popen('/bin/ls -U1 '..directory, 'r')
	local count = 0
	local line = f:read()
	if line then
		repeat
			count = count + 1
			line = f:read()
		until not line
	end
	f:close()
	return count
end

local function get_maildir_counts(maildir)
	local newcount = get_num_files(maildir..'/new/')
	local curcount = get_num_files(maildir..'/cur/')
	return newcount, newcount + curcount
end

local maildir_timer

local function update_maildir()
      for key, maildir in pairs(settings.maildirs) do
          local new, total = get_maildir_counts(maildir)
	  statusd.inform("maildir_"..key, new.."/"..total)
	  statusd.inform("maildir_"..key.."_new", tostring(new))
	  statusd.inform("maildir_"..key.."_total", tostring(total))
	  if new>0 then
	  	statusd.inform("maildir_"..key.."_hint", "important")
		statusd.inform("maildir_"..key.."_new_hint", "important")
		statusd.inform("maildir_"..key.."_total_hint", "important")
	  else
	  	statusd.inform("maildir_"..key.."_hint", "normal")
		statusd.inform("maildir_"..key.."_new_hint", "normal")
		statusd.inform("maildir_"..key.."_total_hint", "normal")
	  end
      end
      maildir_timer:set(settings.interval, update_maildir)
end

maildir_timer = statusd.create_timer()
update_maildir()
