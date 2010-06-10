-- statusd_weather.lua

-- statusd_weather.lua: an Ion3 statusd applet for displaying weather information
-- based on weather.lua by Andrea Rossato arossato AT istitutocolli DOT org

-- Here's the list of all available data:
-- %weather_location
-- %weather_country
-- %weather_date
-- %weather_time (UTC)
-- %weather_timestamp  (timestamp of the latest report: your local time.)
-- %weather_tempF (Fahrenheit)
-- %weather_tempC (Celsius)
-- %weather_dewpointF (Fahrenheit)
-- %weather_dewpointC (Celsius)
-- %weather_humidity
-- %weather_pressure (hPa)
-- %weather_wind
-- %weather_windspeed (MPH)
-- %weather_windchillF (Fahrenheit)
-- %weather_windchillC (Celsius)
-- %weather_sky
-- %weather_weather

-- LEGAL
-- Copyright (C) 2006  Andrea Rossato arossato AT istitutocolli DOT org
-- Copyright (C) 2008  Sergey Kozhemyakin luc AT server DOT by

-- This program is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.

-- This software is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

-- Have a nice weather :)

local defaults = {
   update_interval = 30 * 60 * 1000, -- every 30 minutes
   station = "UMGG",
   timestamp_format = "%F %H:%M", -- strftime format
   gmt_offset = nil, -- offset from gmt in seconds. should be calculated automaticaly but may be specified manualy
}

local settings = table.join(statusd.get_config("weather"), defaults)

local weather_timer = statusd.create_timer()
local wget_timer = statusd.create_timer()

local get_weather

local function calculate_gmt_offset()
   -- offset (+|-)(\d\d)(\d\d)
   local off = {}
   _, _, off.sign, off.hour, off.min = string.find(os.date("%z", os.time()), "([-+])(%d%d)(%d%d)")
   settings.gmt_offset = (off.hour * 60 + off.min) * 60
   if off.sign == '-' then settings.gmt_offset = -settings.gmt_offset end
end

local function reformat_time(date, time)
   local dt = {}
   _, _, dt.year, dt.month, dt.day = string.find(date, "(%d+)%.(%d+)%.(%d+)")
   _, _, dt.hour, dt.min = string.find(time, "(%d%d)(%d%d)")
   os.setlocale(os.getenv("LANG"), "time")
   return os.date(settings.timestamp_format, os.time(dt) + settings.gmt_offset)
end

local function parse_weather(s)
   local w = {}
   _, _, w.location, w.country = string.find(s, "^(.+)%,%s(.+)%(%u+%)" )
   _, _, w.date, w.time = string.find(s, ".+%/%s([%d.]+)%s(%d+)%sUTC" )
   _, _, w.wind, w.windspeed = string.find(s, "Wind%:%s(.+%sat%s(%d+)%sMPH)" )
   _, _, w.sky = string.find(s, "Sky%sconditions:%s(.-)%c" )
   _, _, w.tempF, w.tempC = string.find(s, "Temperature:%s(-?%d*%.?%d+)%sF%s%((-?%d*%.?%d+)%sC%)%c" )
   _, _, w.dewpointF, w.dewpointC = string.find(s, "Dew%sPoint:%s(-?%d*%.?%d+)%sF%s%((-?%d*%.?%d+)%sC%)" )
   _, _, w.windchillF, w.windchillC = string.find(s, "Windchill:%s(-?%d*%.?%d+)%sF%s%((-?%d*%.?%d+)%sC%)" )
   _, _, w.humidity = string.find(s, "Relative%sHumidity:%s(%d+)%%")
   _, _, w.pressure = string.find(s, "Pressure%s%b():.-%((%d+)%shPa%)" )
   _, _, w.weather = string.find(s, "Weather:%s(.-)%c" )
   if w.date and w.time then
      w.timestamp = reformat_time(w.date, w.time)
   else
      w.timestamp = "undef"
   end
   return w
end

local function load_weather()
   local f = io.open ("/tmp/".. settings.station..".dat")
   if not f then return nil end
   local s = f:read("*all")
   f:close()
   os.execute("rm ".."/tmp/".. settings.station..".dat")
   return parse_weather(s)
end

local function update_weather()
   local w = load_weather()
   if w then
      for i,v in pairs(w) do
         if not v then v = "N/A" end
         statusd.inform("weather_"..i, v);
      end
   end
   weather_timer:set(settings.update_interval, get_weather)
end

local function check_wget()
   local filename = "/tmp/wget_"..settings.station.."_status.txt"
   local f = io.open (filename)
   if not f then
       weather_timer:set(settings.update_interval, get_weather)
       return nil
   end
   local s = f:read("*all")
   f:close()
   if not string.match(s, "100%%") then
      -- wget still downloading. resetting timer
      wget_timer:set(2000, check_wget)
   else
      -- wget finished. so we need parse file and update info
      os.execute("rm "..filename)
      update_weather()
   end
end

get_weather = function ()
                 local url = "http://weather.noaa.gov/pub/data/observations/metar/decoded/"
                 local command = "wget -b -o /tmp/wget_"..settings.station.."_status.txt -O /tmp/"..settings.station..".dat "..url..settings.station..".TXT"
                 os.execute(command)
                 -- start watching timer -- when wget will exit -- update weather
                 wget_timer:set(2000, check_wget)
              end

if not settings.gmt_offset then
   calculate_gmt_offset()
end
get_weather()
