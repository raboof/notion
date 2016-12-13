-- Authors: Hendrik Iben <hiben@tzi.de>
-- License: GPL, version 2
-- Last Changed: 2006-03-23
--
-- statusd_mocmon.lua v20060323
--
-- Just another monitor for moc.
-- Believe it or not, while I was working on fixing the crash-bug in
-- statusd_xmmsip.lua I stumbled over statusd_mocp.lua for popen_bgread.
-- This was the moment I realized a) I finally found an audio player I really
-- and fully like (I had my journey on console players the day before) and
-- b) I should not have written xmmsip as I am going to moc up my life... :-)
--
-- But when I took a closer look at the moc scripts I really did not like what
-- they offer and adapting the mocmon stuff for them is quite easy...
--
-- Written by Hendrik Iben < hiben at tzi dot de >
--
-- How to use :
-- mocmon provides a lot of statusd-monitors
-- You can compose the text displayed in your statusbar using these individual
-- monitors or/and you may specify a string with a setup of the values you
-- are interested in.
--
-- The monitors :
--  mocmon_file      : Current file played
--  mocmon_title     : Current file's title
--  mocmon_songtitle : Current song's title
--  mocmon_artist    : Artist performing current song
--  mocmon_album     : Album of current song
--  mocmon_sectime   : Length of file in seconds (0 for streams)
--  mocmon_time      : Length of file in minutes:seconds or '>>>' for streams
--  mocmon_secpos    : Position in file in seconds
--  mocmon_pos       : Position in file in minutes:seconds
--  mocmon_secleft   : Time left in seconds (-1 for streams)
--  mocmon_left      : Time left in minutes:seconds or "<<<" for streams
--  mocmon_state     : moc's status : (PLAY, PAUSE, STOP, NOT RUNNING)
--  mocmon_sstate    : same as above but all lowercase
--  mocmon_bitrate   : Current bitrate
--  mocmon_rate      : Sampling frequency
--
--  mocmon_user      : String resulting from 'user_format'-string
--  			  or 'stopped'-string
--
--  While it is perfectly legal to use these directly in your statusbar I
--  recommend you to only use mocmon_user and configure the format string.
--  This way you do not get a '?' for every monitor when moc is not running...
--
--  Setting up the 'user_format'-string is done by forming a string where each
--  mocmon_x-monitor is referenced to by '%x%'
--
--  example :
--  	"...%mocmon_state %mocmon_title ..blub..." in cfg_statusbar template
--  	
--  ->  "%state% %title%" in mocmon-config 'user_format'
--  and "...%mocmon_user ..blub..." in cfg_statusbar template
--
--  The mentionend 'stopped'-string is specified in mocmon-config as
--  'stopped' and will replace %mocmon_user% whenever moc is stopped or
--  mocp is not found.
--
--  If you are finally convinced that mocmon_user is the only monitor you need
--  you may disable telling statusd about the other monitors. I have no idea, if
--  this has much impact on performance but who knows...
--
--  mocmon-config -> do_monitors = false
--
--  speaking of configuration you might want a description of what the settings
--  do and what the defaults are :
--
--  mocinfo      : command to query moc (mocp -i)
--  interval     : delay between checking moc (5 seconds)
--  do_monitors  : inform statusd of all monitors or just of mocmon_user (true)
--  user_format  : template for mocmon_user (see in code)
--  stoppped     : replacement for mocmon_user when mocp is not found or moc
--                 is stoppped (see in code)
--
--  Requirements :
--  ion3 (tested with 20060305 gentoo ebuild)
--  lua (tested with 5.0.2)
--  moc (tested with 2.4.0)
--
--  Serving suggestions :
--  I use the rotate_statusbar replacement for the standard statusbar as I
--  want to display a lot of things but not always...
--  moc-status information takes quite a bit of space so you might consider
--  doing it a bit like me .. but of course in your own special and
--  unique way. ;-)
--
--  This is my current setup :
--
--  cfg_statusbar (rotate_statusbar)
--
--  ...
--  rotate_statusbar.configurations = {
--   ...
--   ...
--   mocmon = {
--     interval = 1 * 1000,
--     user_format = "(%sstate%) %artist% - %songtitle% (%pos%/%left%/%time%)",
--     stoppped = "MOC up your life!",
--     do_monitors = false,
--   },
--   ...
--   ...
--
--   rotate_statusbar.settings = {
--     ...
--     ...
--     all_statusbars = {
--       "[ %date || MOC %mocmon_user ]%filler%systray",
--       "[ %fortune ]%filler%systray",
--     ...
--     ...
--
--  This setup updates the information every second showing the status of
--  moc, the current title, the time that is left and the total time
--  of the current song.
--  Additionally when moc is not running a get a reminder to turn it on,
--  and as I do not need the other monitors they are disabled.
--
--  Happy listening!
--
--  Feel free to contact me if you discover bugs or want to comment on this.
--
--
-- You are free to distribute this software under the terms of the GNU
-- General Public License Version 2.

if not statusd_mocmon then
  statusd_mocmon = {
    interval=5*1000,
    do_monitors = true,
    user_format = "(%sstate%) %artist% - %songtitle% (%pos%/%left%/%time%)",
    stopped = "moc is stopped...",
    mocinfo = "mocp -i",
    }
end

-- merge external settings with defaults
local settings = table.join (statusd.get_config("mocmon"), statusd_mocmon)

-- mapping to moc-Info
local valueassoc = {
  mocmon_file = "File",
  mocmon_title = "Title",
  mocmon_songtitle = "SongTitle",
  mocmon_artist = "Artist",
  mocmon_album = "Album",
  mocmon_sectime = "TotalSec",
  mocmon_time = "TotalTime",
  mocmon_secpos = "CurrentSec",
  mocmon_pos = "CurrentTime",
  mocmon_left = "TimeLeft",
  mocmon_secleft = "secleft",
  mocmon_state = "State",
  mocmon_sstate = "sstate",
  mocmon_bitrate = "Bitrate",
  mocmon_rate = "Rate",
  }

-- changes all 'nil's in the infotable to '?' (for statusbar)
-- needed when the infopipe is not running or incompatible...
local function fixTable(it)
  for _, v in pairs(valueassoc)
    do
      if it[v] == nil
        then
	  it[v] = "?"
      end
    end
  return it
end

-- some convenience values
local function addSpecialValues(it)
  local state = it[valueassoc["mocmon_state"]]
  if state ~= nil
    then
      it[valueassoc["mocmon_sstate"]] = string.lower(state)
  end

  local sectime = it[valueassoc["mocmon_sectime"]]
  local left = it[valueassoc["mocmon_left"]]

  if (not sectime) or (sectime == "")
    then
      it[valueassoc["mocmon_sectime"]]=0
      it[valueassoc["mocmon_time"]]=">>>"
  end

  if (not left) or (left == "")
    then
      it[valueassoc["mocmon_left"]]="<<<"
      it[valueassoc["mocmon_secleft"]]=-1
    else
      local _, _, min, sec = string.find(left, "([0-9]+):([0-9]+)")
      if min and sec
        then
	  it[valueassoc["mocmon_secleft"]]=min*60+sec
	else
	  it[valueassoc["monmon_secleft"]]=0
      end
  end

  return it
end

-- this formats the user_format-string
local function makeUserString(s, it)
  local rval = s
  for k, v in pairs(valueassoc)
    do
      -- it would be annoying to type 'mocmon_' in front
      -- of everything...
      _, _, stripped = string.find(k, "mocmon_(.*)")
      rval = string.gsub(rval, "%%"..stripped.."%%", it[v])
  end
  return rval
end

-- our update timer
local mocmon_timer

-- retrive information from moc - if found.
-- calculate some additional values and clean the table
-- from nilS
-- After that update statusd-monitors if the users wishes
-- and create the user-string from the template
local function fetch_data(partial_data)
  local infotable = { } -- tabula rasa
  local mocdata = ""
  while partial_data
    do
      mocdata = mocdata .. partial_data
      partial_data = coroutine.yield()
    end

  local running = true -- assume the best

  if mocdata and mocdata ~= ""
    then
      for attribute, value in string.gmatch(mocdata, "([^:]*):([^\n]*)\n")
        do
	  infotable[attribute] = string.gsub(value, "^%s*(.*)", "%1")
	end
    else
      -- obvious...
      running = false
      infotable[valueassoc["mocmon_state"]]="NOT RUNNING"
  end

  -- compute things like time left or divide things by 1000...
  infotable = addSpecialValues(infotable)
  -- scan for nil-values and fix them with '?'
  infotable = fixTable(infotable)

  local stopped = false
  if infotable[valueassoc["mocmon_state"]] == "STOP"
    then
      stopped = true
  end

  -- if we are to update the monitors...
  if settings.do_monitors
    then
      for k, v in pairs(valueassoc)
        do
	  -- do so, but coerce to string
          statusd.inform(k, ""..infotable[v])
        end
  end

  -- create appropriate user-string
  if running and (not stopped)
    then
      statusd.inform("mocmon_user", makeUserString(settings.user_format, infotable))
    else
      statusd.inform("mocmon_user", settings.not_running)
  end

  mocmon_timer:set(settings.interval, update_mocmon)
end

-- fetch error (and discard...)
local function flush_stderr(partial_data)
  local result = ""
  while partial_data
    do
      result = result .. partial_data
      partial_data = coroutine.yield()
    end

  -- I don't know what to do with the actual error...
  -- This would be called a lot when mocp is not
  -- found...
end

-- update monitor with bgread
function update_mocmon()
  statusd.popen_bgread(
      settings.mocinfo
    , coroutine.wrap(fetch_data)
    , coroutine.wrap(flush_stderr)
  )
end

-- intialize timer
mocmon_timer = statusd.create_timer()
-- start updating
update_mocmon()

