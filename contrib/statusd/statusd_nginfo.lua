-- Authors: Raffaello Pelagalli <raffa@niah.org>
-- License: LGPL, version 2.1 or later
-- Last Changed: 2008-05-08
--
-- statusd_nginfo.lua
-- 
-- Made by Raffaello Pelagalli (raffa at niah.org)
-- 
-- Started on  Sun Mar  9 00:22:31 2008 Raffaello Pelagalli
-- Last update Thu May  8 23:29:32 2008 Raffaello Pelagalli
-- 
-- This library is free software; you can redistribute it and/or
-- modify it under the terms of the GNU Lesser General Public
-- License as published by the Free Software Foundation; either
-- version 2.1 of the License, or (at your option) any later version.
-- 
-- This library is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
-- Lesser General Public License for more details.
-- 
-- You should have received a copy of the GNU Lesser General Public
-- License along with this library; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
-- 02110-1301  USA
--

-- Nagios checking script
-- Reports nagios status in ion status bar
-- Sample configuration:
-- mod_statusbar.launch_statusd{
--    ...
--    nginfo = {
--       urls = {
--          "http://user1:password1@server1.domain1.tld/cgi-bin/nagios2/nginfo.pl",
--          "http://user2:password2@server2.domain2.tld/nagios/cgi-bin/nginfo.pl",
--       },
--    }
--    ...
-- }
--
-- Need to be used with nginfo.pl script from 
-- http://redstack.net/blog/index.php/2008/05/08/nagios-status-report-in-ion3-statusbar.html

require "lxp"
local ng_timer
local error = false

local status = {0, 0, 0, 0}

local defaults = { 
   update_interval=30*1000, 
   urls = { },
}

local settings = table.join(statusd.get_config("nginfo"), defaults)

nginfo_callbacks = {
   StartElement = function (parser, name)
                     if (name == "current_state") then
                        nginfo_callbacks.CharacterData = function (parser, val)
                                                            status[tonumber(val) + 1] = 
                                                               status[tonumber(val) + 1] + 1
                                                         end
                     end
                  end,
   EndElement = function (parser, name)
                   if (name == "current_state") then
                      nginfo_callbacks.CharacterData = false
                   end
                end,
   CharacterData = false,
}

function parse (data)
   p = lxp.new(nginfo_callbacks)
   p:parse(b)
   p:close()
end

function get_nginfo ()
   status = {0, 0, 0, 0}
   error = false
   local http = require("socket.http")
   socket.http.TIMEOUT=10
   local errstr = " ERROR while reading data"
   for n, url in pairs(settings.urls) do
      b, c, h = http.request(url)
      if not (c == 200) then 
         error = true 
         errstr = errstr .. " (NET " .. tostring(c) .. ")"
      else
         local st, err = pcall(parse, b)
         if not st then 
            error = true 
            errstr = errstr .. " (XML" .. err .. ")"
         end
      end
   end
   
   if not error then
      errstr = ""
   end
   return "OK: " .. tostring(status[1])
      .. ", WARN: " .. tostring(status[2])
      .. ", ERROR: " .. tostring(status[3])
      .. ", UNKN: " .. tostring(status[4])
      .. errstr
end

local function update_nginfo()
   statusd.inform("nginfo", get_nginfo())
   if (status[3] > 0 or status[4] > 0) then
      statusd.inform("nginfo_hint", "critical")
   elseif (status[2] > 0) then
      statusd.inform("nginfo_hint", "important")
   else 
      statusd.inform("nginfo_hint", "normal")
   end
   ng_timer:set(settings.update_interval, update_nginfo)
end

-- Init
ng_timer=statusd.create_timer()
update_nginfo()
