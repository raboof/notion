-- Authors: Sadrul Habib Chowdhury <imadil@gmail.com>
-- License: Public domain
-- Last Changed: Unknown
--
-- statusd_uptime.lua
--
-- Author
-- Sadrul Habib Chowdhury (Adil)
-- imadil at gmail dot com

--
-- how often should the monitor be updated?
--

if not statusd_uptime then
  statusd_uptime={
    interval=30*1000,
  }
end

local timer = nil   -- the timer

--
-- update the uptime monitor
--
local function get_uptime_info()
    local f=io.popen('uptime', 'r')
    timer:set(statusd_uptime.interval, get_uptime_info)
    if not f then
        statusd.inform("uptime", "oops")
        return
    end
    local s=f:read('*line')
    f:close()

    s = string.gsub(s, ", +%d+ user.+", "")    -- unnecessary
    s = string.gsub(s, "%d+:%d+:%d+ up +", "") -- time is unnecessary

    statusd.inform("uptime", s)
end

--
-- start the timer
--
local function init_uptime_monitor()
    timer = statusd.create_timer()
    statusd.inform("uptime_template", "xxxxxxxxxxxxxxx")
    get_uptime_info()
end

init_uptime_monitor()

