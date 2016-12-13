-- Authors: Jari Eskelinen <jari.eskelinen@iki.fi>, René van Bevern <rvb@pro-linux.de>, Juri Hamburg <juri@fail2fail.com>, Philipp Hartwig <ph@phhart.de>
-- License: GPL, version 2
-- Last Changed: 2013-04-09
--
-- statusd_laptopstatus.lua v0.0.3 (last modified 2011-10-30)
--
-- Copyright (C) 2005 Jari Eskelinen <jari.eskelinen@iki.fi>
--  modified by René van Bevern <rvb@pro-linux.de> for error handling
--  modified by Juri Hamburg <juri@fail2fail.com> for sysfs support (2011)
--  modified by Philipp Hartwig <ph@phhart.de> for improved sysfs support (2013)
--
-- Permission to copy, redistirbute, modify etc. is granted under the terms
-- of GNU GENERAL PUBLIC LICENSE Version 2.
--
-- This is script for displaying some interesting information about your
-- laptops power saving in Notion's status monitor. Script is very Linux
-- specific (uses procfs or sysfs interface) and needs ACPI-support (don't
-- know exactly but 2.6.x kernels should be fine). Also if you have some
-- kind of exotic hardware (multiple processors, multiple batteries etc.)
-- this script probably will fail or show incorrect information.
--
-- The script will try sysfs interface first. If that fails, it will try to
-- use the procfs interface.
--
-- Just throw this script under ~/.notion and add following keywords to your
-- cfg_statusbar.lua's template-line with your own taste:
--
-- %laptopstatus_cpuspeed
-- %laptopstatus_temperature
-- %laptopstatus_batterypercent
-- %laptopstatus_batterytimeleft
-- %laptopstatus_batterydrain
--
-- Template example: template="[ %date || load:% %>load || CPU: %laptopstatus_cpuspeed %laptopstatus_temperature || BATT: %laptopstatus_batterypercent %laptopstatus_batterytimeleft %laptopstatus_batterydrain ]"
--
-- You can also run this script with lua interpreter and see if you get
-- right values.
--
-- TODO: * Is it stupid to use file:read("*all"), can this cause infinite
--         loops in some weird situations?
--       * Do not poll for information not defined in template to be used

--
-- SETTINGS
--

NA = "n/a"
AC = "*AC*"

sys_dir="/sys/class/"
proc_dir="/proc/acpi/"

if not statusd_laptopstatus then
  statusd_laptopstatus = {
    interval = 10,                    -- Polling interval in seconds
    temperature_important = 66,       -- Temperature which will cause important hint
    temperature_critical = 71,        -- Temperature which will cause critical hint
    batterypercent_important = 10,    -- Battery percent which will cause important hint
    batterypercent_critical = 5,      -- Battery percent which will cause critical hint
    batterytimeleft_important = 600,  -- Battery time left (in secs) which will cause important hint
    batterytimeleft_critical = 300,   -- Battery time left (in secs) which will cause critical hint
  }
end

if statusd ~= nil then
    statusd_laptopstatus=table.join(statusd.get_config("laptopstatus"), statusd_laptopstatus)
end

--
-- CODE
--
local laptopstatus_timer
local RingBuffer = {}

RingBuffer.__index = RingBuffer

function RingBuffer.create(size)
    local buf = {}
    setmetatable(buf,RingBuffer)
    buf.size = size
    buf.elements = {}
    buf.current = 1
    return buf
end

function RingBuffer:add(val)
   if self.current > self.size then
       self.current = 1
   end
   self.elements[self.current] = val
   self.current = self.current + 1
end

function average(array)
    local sum = 0
    for i,v in ipairs(array) do
        sum = sum + v
    end
    if #array ~= 0 then
       return sum / #array
   else
       return 0
   end
end

rates_buffer = RingBuffer.create(20)


function read_val(file)
    local fd = io.open(file)
    if fd then
        local value = fd:read("*a")
        fd:close()
        return value
    end
end

-- Returns the full path of the first subdirectory of "path" containing a file
-- with name "filename" and first line equal to "content"
function lookup_subdir(path, filename, content)
    if path:sub(-1) ~= "/" then path = path .. "/" end
    local fd=io.open(path)
    if not fd then return nil end
    fd:close()

    function dir_iter()
        if pcall(require, "lfs") then
            return lfs.dir(path)
        else
            return io.popen("ls -1 " .. path):lines()
        end
    end

    for dir in dir_iter() do
        if not (dir == "." or dir == "..") then
            local fd=io.open(path .. dir .. "/" .. filename)
            if fd and ((not content) or content == fd:read():lower()) then
                return path .. dir .. "/"
            end
        end
    end
end

local function get_cpu()
  local mhz, hint
  if pcall(function ()
       local status
       status, _, mhz = string.find(read_val("/proc/cpuinfo"),
                                    "cpu MHz%s+: (%d+)")
       if not status then error("could not get MHz") end
     end)
     then return {speed=string.format("%4dMHz", math.ceil(mhz/5)*5),
                  hint=hint}
     else return {speed=NA, hint=hint} end
end

local function on_ac()
  if ac_sys then
      return (tonumber(read_val(ac_dir .. "online") or "") == 1)
  else
      return string.find(read_val(ac_dir .. "state") or "", "state:%s+on.line")
  end
end

local function get_thermal_sysfs()
    local temp, hint = nil, "normal"

    if pcall(function ()
        temp=read_val(temp_dir .. "temp")
        temp = tonumber(temp)/1000
    end)
     then if temp >= statusd_laptopstatus.temperature_important then
             hint = "important" end
          if temp >= statusd_laptopstatus.temperature_critical then
             hint = "critical" end
          return {temperature=string.format("%02dC", temp), hint=hint}
     else return {temperature=NA, hint = "hint"} end
 end

local function get_thermal_procfs()
  local temp, hint = nil, "normal"

  if pcall(function ()
       local status
       status, _, temp = string.find(read_val(temp_dir .. "temperature"),
                                     "temperature:%s+(%d+).*")
       if not status then error("could not get temperature") end
       temp = tonumber(temp)
     end)
     then if temp >= statusd_laptopstatus.temperature_important then
             hint = "important" end
          if temp >= statusd_laptopstatus.temperature_critical then
             hint = "critical" end
          return {temperature=string.format("%02dC", temp), hint=hint}
     else return {temperature=NA, hint = "hint"} end
end

local function get_thermal()
    if temp_sys then
        return get_thermal_sysfs()
    else
        return get_thermal_procfs()
    end
end


local rate_sysfs
local last_power

local function get_battery_sysfs()
    local percenthint = "normal"
    local timelefthint = "normal"
    local full, now

    if pcall(function ()
        now = tonumber(read_val(bat_dir .. "energy_now"))
        full = tonumber(read_val(bat_dir .. "energy_full"))
    end)
    or
    pcall(function ()
        now = tonumber(read_val(bat_dir .. "charge_now"))
        full = tonumber(read_val(bat_dir .. "charge_full"))
    end)
    then
        --Some batteries erronously report a charge_now value of charge_full_design when full.
        if now > full then now=full end
        local percent = math.floor(now / full * 100 + 5/10)
        local timeleft
        if on_ac() then
            timeleft = AC
        elseif last_power ~= nil and now < last_power then
            rate_sysfs = last_power - now
            rates_buffer:add(rate_sysfs)
            secs = statusd_laptopstatus.interval * (now / average(rates_buffer.elements))
            mins = secs / 60
            hours = math.floor(mins / 60)
            mins = math.floor(mins - (hours * 60))
            timeleft = string.format("%02d:%02d", hours, mins)
        else
            timeleft = NA
        end
        last_power = now

       if percent <= statusd_laptopstatus.batterypercent_important then percenthint = "important" end
       if percent <= statusd_laptopstatus.batterypercent_critical then percenthint = "critical" end
       return { percent=string.format("%02d%%", percent), timeleft=timeleft, drain=NA, percenthint=percenthint, timelefthint=timelefthint}
    else return { percent=NA, timeleft=NA, drain=NA, percenthint=percenthint, timelefthint=timelefthint} end
end

local function get_battery_procfs()
  local percenthint = "normal"
  local timelefthint = "normal"
  local lastfull, rate, rateunit, remaining

  if pcall(function ()
       local status
       local statecontents = read_val(bat_dir .. "state")

       status, _, lastfull = string.find(read_val(bat_dir .. "info"), "last full capacity:%s+(%d+).*")
       if not status then error("could not get full battery capacity") end
       lastfull = tonumber(lastfull)
       if string.find(statecontents, "present rate:%s+unknown.*") then
           rate = -1
       else
           status, _, rate, rateunit = string.find(statecontents, "present rate:%s+(%d+)(.*)")
           if not status then error("could not get battery draining-rate") end
           rate = tonumber(rate)
       end
       status, _, remaining = string.find(statecontents, "remaining capacity:%s+(%d+).*")
       if not status then error("could not get remaining capacity") end
       remaining = tonumber(remaining)
     end) then
       local percent = math.floor(remaining / lastfull * 100 + 5/10)
       local timeleft
       local hours, secs, mins
       if on_ac() then
           timeleft = AC
       elseif rate <= 0 then
           timeleft = NA
       else
          secs = 3600 * (remaining / rate)
          mins = secs / 60
          hours = math.floor(mins / 60)
          mins = math.floor(mins - (hours * 60))
          timeleft = string.format("%02d:%02d", hours, mins)
       end

       if secs ~= nil and secs <= statusd_laptopstatus.batterytimeleft_important then timelefthint = "important" end
       if secs ~= nil and secs <= statusd_laptopstatus.batterytimeleft_critical then timelefthint = "critical" end
       if percent <= statusd_laptopstatus.batterypercent_important then percenthint = "important" end
       if percent <= statusd_laptopstatus.batterypercent_critical then percenthint = "critical" end

       return { percent=string.format("%02d%%", percent), timeleft=timeleft, drain=tostring(rate)..rateunit, percenthint=percenthint, timelefthint=timelefthint}
  else return { percent=NA, timeleft=NA, drain=NA, percenthint=percenthint, timelefthint=timelefthint} end
end

local function get_battery()
    if bat_sys then
        return get_battery_sysfs()
    else
        return get_battery_procfs()
    end
end

local last_timeleft = nil

function do_init()
    init=true

    ac_dir=lookup_subdir(sys_dir .. "power_supply" , "type", "mains")
    ac_sys=true
    if(not ac_dir) then
        ac_dir=lookup_subdir(proc_dir .. "ac_adapter", "state")
        ac_sys=false
    end

    bat_dir=lookup_subdir(sys_dir .. "power_supply", "type", "battery")
    bat_sys=true
    if(not bat_dir) then
        bat_dir=lookup_subdir(proc_dir .. "battery", "state")
        bat_sys=false
    end

    temp_dir=lookup_subdir(sys_dir .. "thermal", "type", "acpitz") or lookup_subdir(sys_dir .. "thermal", "type", "thermal zone")
    temp_sys=true
    if(not temp_dir) then
        temp_dir=lookup_subdir(proc_dir .. "thermal_zone", "temperature")
        temp_sys=false
    end
end

local function update_laptopstatus ()
  if not init then do_init() end
  cpu = get_cpu()
  thermal = get_thermal()
  battery = get_battery()

  -- Informing statusd OR printing to stdout if statusd not present
  if statusd ~= nil then
    statusd.inform("laptopstatus_cpuspeed", cpu.speed)
    statusd.inform("laptopstatus_cpuspeed_template", "xxxxMHz")
    statusd.inform("laptopstatus_cpuspeed_hint", cpu.hint)
    statusd.inform("laptopstatus_temperature", thermal.temperature)
    statusd.inform("laptopstatus_temperature_template", "xxxC")
    statusd.inform("laptopstatus_temperature_hint", thermal.hint)
    statusd.inform("laptopstatus_batterypercent", battery.percent)
    statusd.inform("laptopstatus_batterypercent_template", "xxx%")
    statusd.inform("laptopstatus_batterypercent_hint", battery.percenthint)
    if battery.timeleft ~= NA or last_timeleft == AC then
        statusd.inform("laptopstatus_batterytimeleft", battery.timeleft)
        last_timeleft = battery.timeleft
    end
    statusd.inform("laptopstatus_batterytimeleft_template", "hh:mm")
    statusd.inform("laptopstatus_batterytimeleft_hint", battery.timelefthint)
    statusd.inform("laptopstatus_batterydrain", battery.drain)
    laptopstatus_timer:set(statusd_laptopstatus.interval*1000, update_laptopstatus)
  else
    io.stdout:write("CPU: "..cpu.speed.." "..thermal.temperature.." || BATT: "..battery.percent.." "..battery.timeleft.."\n")
  end
end


if statusd ~= nil then
    laptopstatus_timer = statusd.create_timer()
end
update_laptopstatus()
