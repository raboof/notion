-- Authors: Andrea Rossato <arossato@istitutocolli.org>
-- License: GPL, version 2 or later
-- Last Changed: 2006-07-07
--
-- weather.lua

--
-- NOTE: this script should be rewritten as a statusd script
--

-- weather.lua: an Ion3 applet for displaying weather information

-- INTRODUCTION
-- I discovered Ion last week, after reading an article on linux.com. I
-- installed it and, in a matter of a few hours, I decided to stick to
-- it. How can I say: I've been dreaming of a WM to be run from the
-- command line... and this is what Ion3 can do. And what a kind of a
-- command line! A Lua interpreter! I must confess I've never heard about
-- Lua before, and it was quite a surprise (I had the same feeling with
-- JavaScript some time ago).
-- I decided to write this applet mostly to learn and explore Lua,
-- especially for object-oriented programming.

-- ABOUT
-- This applet can be used to monitor the weather condition(s) of one or
-- more weather observation station(s). Data will be retrieved from
-- http://weather.noaa.gov and displayed in the statusbar.

-- USAGE
-- You need to dopath() this script (e.g. from cfg_ion3.lua):
-- dopath("weather")

-- - To monitor one station you can insert, in your cfg_statusbar.lua, within
-- mod_statusbar.launch_statusd{}, something like:
-- weather = {
--    station = "KNYC",
-- }
-- In your template insert something like:
-- %weather_location: %weather_tempC %weather_humidity (%weather_time)

-- Here's the list of all available data:
-- %weather_location
-- %weather_country
-- %weather_date
-- %weather_time (time of the latest report: your local time)
-- %weather_tempF (Fahrenheit)
-- %weather_tempC (Celsius)
-- %weather_dewpointF (Fahrenheit)
-- %weather_dewpointC (Celsius)
-- %weather_humidity
-- %weather_pressure (hPa)
-- %weather_wind
-- %weather_windspeed (MPH)
-- %weather_sky
-- %weather_weather

-- - If you want to monitor more stations you need to create a monitor
-- object for each one with the new_wm() function. After
-- dopath("weather") write something like:
-- mymonitor1 = new_wm("KNYC")
-- mymonitor2 = new_wm("LIBP")

-- You can create a new monitor also at run time. Get the Lua code prompt
-- (usually META+F3) and run:
-- mymonitor3 = new_wm("LIBC")

-- Do not set any station in cfg_statusbar.lua, since that station would
-- be used by *all* monitors.
-- Each monitor will output the data either in %weater_meter_XXXX, where XXXX is
-- the station code (KNYC), and in %weather_meter.

-- CONFIGURATION
-- Default configuration values, that will apply to *all* monitors, can be
-- written in cfg_statusbar.lua in your ~/.ion3 directory.

-- Each monitor can be also configured at run-time.
-- For instance: if you are monitoring only one station, get the Lua code
-- prompt (usually META+F3) and run:
-- WeatherMonitor.config.unit.tempC = "C"
-- or
-- WeatherMonitor.config.critical.tempC = "15"
-- You can save run-time configuration with the command:
-- WeatherMonitor.save_config()
-- This configuration will not overwrite the default station to be
-- monitored.

-- COMMANDS
-- Monitors are objects that export some public methods. You can use
-- these methods to change the state of the objects.
-- You can change configuration value and save them (see CONFIGURATION)
-- or issues some commands.

-- Suppose you have 2 monitors:
-- mon1 = new_wm("LIBP")
-- mon2 = new_wm("KNYC")
-- (Remember: in single mode the name of the object is WeatherMonitor.)

-- Get the Lua code prompt (usually META+F3) and:
-- 1. to force one monitor to update the data:
-- mon1.update()

-- 2. to show the full report of the station:
-- mon2.show_data(_)

-- 3. to update data and show the report:
-- WeatherMonitor.update_and_show(_)

-- 4. to chnage station (only for run-time):
-- WeatherMonitor.set_station(_)

-- 5. to change one monitor configuration:
-- mon1.config.critical.tempC = "15"

-- Obviously you can create key-bindings to run these commands.

-- REVISIONS
-- 2006-07-07 first release

-- LEGAL
-- Copyright (C) 2006  Andrea Rossato

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
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA


-- Have fun!

-- Andrea Rossato arossato AT istitutocolli DOT org


function new_wm(station)
   local this = {
      config = {
	 station = station,
	 interval =30 * 60 * 1000,   -- check every 30 minutes
	 unit = {
	    tempC = "°",
	    tempF = "F",
	    humidity = "%",
	    pressure = "hPa",
	    windspeed = "MPH",

	 },
	 -- Threshold information.  These values should likely be tweaked to
	 -- suit local conditions.
	 important = {
	    tempC = 10,
	    tempF = 50,
	    windspeed = 5,
	    humidity = 30,
	 },
	 critical = {
	    tempC = 26,
	    tempF = 78,
	    windspeed = 20,
	    humidity = 60,
	 },
      },
      status_timer = ioncore.create_timer(),
      url = "http://weather.noaa.gov/pub/data/observations/metar/decoded/",
      paths = ioncore.get_paths(),
      data = {
	 timezone = tonumber(os.date("%z"))
      },
   }

   -- process configuration:
   -- - config in cfg_statusbar.lua will overwrite default config
   -- - run-time configuration (to be saved at run-time with this.save_config())
   --    will overwrite default config but will be overwtitten by cfg_statusbar.lua
   this.process_config = function()
			    table.merge = function(t1, t2)
					     local t=table.copy(t1, false)
					     for k, v in pairs(t2) do
						t[k]=v
					     end
					     return t
					  end
			    if not this.config.station then this.config.station = "LIPB" end
			    local config = ioncore.read_savefile("cfg_weather_"..this.config.station)
			    if config then
			       this.config = table.merge(this.config, config)
			    end
			    config = ioncore.read_savefile("cfg_statusd")
			    if config.weather then
			       this.config = table.merge(this.config, config.weather)
			    end
			 end

   -- retrive data from server
   this.update_data = function()
                         -- wget options
                         -- -b  go into the background (otherwise it will hang ion startup
                         --     until data file is download)
                         -- -o  output error log (specify filename if you want to save output)
			 local command = "wget -b -o /dev/null -O "..this.paths.sessiondir.."/"..this.config.station..".dat "..
			    this.url..this.config.station..".TXT"
			 os.execute(command)
			 local f = io.open(this.paths.sessiondir .."/".. this.config.station..".dat", "r")
			 if not f then return end
			 local s=f:read("*all")
			 f:close()
			 this.raw_data = s
			 os.execute("rm "..this.paths.sessiondir .."/".. this.config.station..".dat")
		      end

   -- process retrived data and store them in this.data
   this.process_data = function()
			  local s = this.raw_data
			  _, _, this.data.location, this.data.country =
			     string.find(s, "^(.+)%,%s(.+)%(%u+%)" )
			  _, _, this.data.date, this.data.time =
			     string.find(s, ".+%/%s([%d.]+)%s(%d+)%sUTC" )
			  _, _, this.data.wind, this.data.windspeed =
			     string.find(s, "Wind%:%s(.+%sat%s(%d+)%sMPH)" )
			  _, _, this.data.sky = string.find(s, "Sky%sconditions:%s(.-)%c" )
			  _, _, this.data.tempF, this.data.tempC =
			     string.find(s, "Temperature:%s([%d%.]+)%sF%s%(([%d%.]+)%sC%)%c" )
			  _, _, this.data.dewpointF, this.data.dewpointC =
			     string.find(s, "Dew%sPoint:%s([%d%.]+)%sF%s%(([%d%.]+)%sC%)" )
			  _, _, this.data.humidity =
			     string.find(s, "Relative%sHumidity:%s(%d+)%%")
			  _, _, this.data.pressure =
			     string.find(s, "Pressure%s.+%((.+)%shPa%)" )
			  _, _, this.data.weather =
			     string.find(s, "Weather:%s(.-)%c" )
			  this.format_time()
		       end

   -- format teh time string to get hh:mm
   this.format_time = function()
			 local time
			 if this.data.time then
			    time = tonumber(this.data.time) + tonumber(this.data.timezone)
			 else return
			 end
			 if time > 2400 then
			    time = tostring(time - 2400)
			 end
			 if string.match(time, "^%d%d$") then
			    time = "00"..time
			 end
			 if string.match(time, "^%d%d%d$") then
			    time = "0"..time
			 end
			 this.data.time = tostring(time):gsub("(%d%d)(%d%d)","%1%:%2")
		      end

   -- get threshold information
   this.get_hint = function(meter, val)
		      local hint = "normal"
		      local crit = this.config.critical[meter]
		      local imp = this.config.important[meter]
		      if crit and tonumber(val) > crit then
			 hint = "critical"
		      elseif imp and tonumber(val) > imp then
			 hint = "important"
		      end
		      return hint
		   end

   -- get the unit of each meter
   this.get_unit = function(meter)
		      local unit = this.config.unit[meter]
		      if unit then return unit end
		      return ""
		   end

   -- update information for mod_statusbar
   this.notify = function()
		    for i,v in pairs(this.data) do
		       mod_statusbar.inform("weather_"..i.."_"..this.config.station.."_hint", this.get_hint(i, v))
		       mod_statusbar.inform("weather_"..i.."_hint", this.get_hint(i, v))
		       if not v then v = "N/A" end
		       mod_statusbar.inform("weather_"..i.."_"..this.config.station, v..this.get_unit(i))
		       mod_statusbar.inform("weather_"..i, v..this.get_unit(i))
		    end
		    mod_statusbar.update()
		 end
   --
   -- some public methods
   --
   -- save object state (each monitor will store its configuration in
   -- a file named cfg_weather_XXXX where XXXX is the station code
   -- this configuration will apply only to the monitor watching that
   -- station. In other words, you cannot set the default station for
   -- the monitor with this.set_station() (see comments below).
   this.save_config = function()
			 ioncore.write_savefile("cfg_weather_"..this.config.station, this.config)
		      end

   -- restarts the object
   this.update = function()
		    this.init()
		 end

   -- shows full report
   this.show_data = function(mplex)
		       mod_query.message(mplex, this.raw_data)
		    end

   -- updates data and shows updated full report
   this.update_and_show = function(mplex)
			     this.init()
			     this.show_data(mplex)
			  end

   -- changes station. the new station will not be saved:
   -- to change station edit the configuration or start the
   -- monitor with the station as a paramenter, like:
   -- mymon = new_wm("KNYC")
   this.set_station = function(mplex)
			 local handler = function(mplex, str)
					    this.config.station = str;
					    this.init()
					 end
			 mod_query.query(mplex, TR("Enter a station code:"), nil, handler,
					 nil, "weather")
		      end

   -- constructor
   this.init = function()
		  this.update_data()
		  this.process_data()
		  if mod_statusbar ~= nil then
		     this.notify()
		     this.status_timer:set(this.config.interval, this.init)
		  end
	       end

   -- initialize the object
   this.process_config()
   this.init()
   return {
      config = this.config,
      data = this.data,
      save_config = this.save_config,
      update = this.update,
      show_data = this.show_data,
      update_and_show = this.update_and_show,
      set_station = this.set_station,
   }
end

-- start default monitor
WeatherMonitor = new_wm()
