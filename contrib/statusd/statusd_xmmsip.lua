-- statusd_xmmsip.lua v20060323
--
-- Interface to the XMMS InfoPipe-Plugin
-- Inspired by the high cpu attention statusd_xmms.lua + python demand...
-- ( no offense, just observation... :-) 
--   (using statusd_xmms while writing this... ;-) )
-- 
-- Written by Hendrik Iben < hiben at tzi dot de >
--
-- How to use :
-- xmmsip provides a lot of statusd-monitors
-- You can compose the text displayed in your statusbar using these individual
-- monitors or/and you may specify a string with a setup of the values you
-- are interested in.
--
-- The monitors :
--  xmmsip_pllen     : Length of the playlist (number of entries)
--  xmmsip_plcur     : Current song in playlist (number of entry)
--  xmmsip_file      : Current file played
--  xmmsip_title     : Current file's title
--  xmmsip_usectime  : Length of file in milliseconds (-1 for streams)
--  xmmsip_time      : Length of file in minutes:seconds or '>>>' for streams
--  xmmsip_usecpos   : Position in file in milliseconds
--  xmmsip_pos       : Position in file in minutes:seconds
--  xmmsip_usecleft  : Time left in milliseconds (0 for streams)
--  xmmsip_left      : Time left in minutes:seconds or "<<<" for streams
--  xmmsip_status    : XMMS's status : (Playing, Paused, Stopped, Not running)
--  xmmsip_sstatus   : same as above but all lowercase
--  xmmsip_bitrate   : Current bitrate (not average bitrate)
--  xmmsip_kbitrate  : Current bitrate in kilobit per seconds (using 1000 for k)
--  xmmsip_freq      : Sampling frequency in Hz
--  xmmsip_kfreq     : Sampling frequency in kHz
--  xmmsip_channels  : Current channels (1,2,?)
--  xmmsip_xmmsproto : Protocol version of XMMS
--  xmmsip_infover   : Version of InfoPipe-Plugin
--  
--  xmmsip_user		: String resulting from 'user_format'-string
--  			  or 'not_running'-string
--
--  While it is perfectly legal to use these directly in your statusbar I
--  recommend you to only use xmmsip_user and configure the format string.
--  This way you do not get a '?' for every monitor when XMMS is not running...
--
--  Setting up the 'user_format'-string is done by forming a string where each
--  xmmsip_x-monitor is referenced to by '%x%'
--  
--  example :
--  	"...%xmmsip_status %xmmsip_title ..blub..." in cfg_statusbar template
--  	
--  ->  "%status% %title%" in xmmsip-config 'user_format'
--  and "...%xmmsip_user ..blub..." in cfg_statusbar template
--
--  The mentionend 'not_running'-string is specified in xmmsip-config as
--  'not_running' and will replace %xmmsip_user% whenever the XMMS InfoPipe
--  is not running (or found...)
--
--  If you are finally convinced that xmmsip_user is the only monitor you need
--  you may disable telling statusd about the other monitors. I have no idea, if
--  this has much impact on performance but who knows...
--
--  xmmsip-config -> do_monitors = false
--
--  speaking of configuration you might want a description of what the settings
--  do and what the defaults are :
--  
--  interval     : delay between checking the pipe (5 seconds)
--  user         : user name (for xmms-session) (current user)
--  xmms_session : the xmms-session to use (0)
--  do_monitors  : inform statusd of all monitors or just of xmmsip_user (true)
--  user_format  : template for xmmsip_user (see in code)
--  not_running  : replacement for xmmsip_user when no pipe is found
--                 (see in code)
--  
--  Requirements :
--  ion3 (tested with 20060305 gentoo ebuild)
--  lua (tested with 5.0.2)
--  xmms-infopipe v1.3 or compatible
--    I have not tested pipes from/for other players...
--  
--  Serving suggestions :
--  I use the rotate_statusbar replacement for the standard statusbar as I
--  want to display a lot of things but not always...
--  XMMS-status information takes quite a bit of space so you might consider
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
--   xmmsip = {
--     interval = 1 * 1000,
--     user_format = "XMMS (%status%) %title% (%left%/%time%)",
--     not_running = "Turn on the Radio!",
--     do_monitors = false,
--   },
--   ...
--   ...
--
--   rotate_statusbar.settings = {
--     ...
--     ...
--     all_statusbars = {
--       "[ %date || %xmmsip_user ]%filler%systray",
--       "[ %fortune ]%filler%systray",
--     ...
--     ...
--     
--  This setup updates the information every second showing the status of
--  xmms, the current title, the time that is left and the total time 
--  of the current song.
--  Additionally when xmms is not running a get a reminder to turn it on,
--  and as I do not need the other monitors they are disabled.
--
--  Happy listening!
--  
--  Feel free to contact me if you discover bugs or want to comment on this.  
--  
--
-- You are free to distribute this software under the terms of the GNU
-- General Public License Version 2.

if not statusd_xmmsip then
  statusd_xmmsip = {
    interval=5*1000,
    user = os.getenv("USER"),
    xmms_session = 0,
    do_monitors = true,
    user_format = "%status%: %title% (%pos%/%time%, %plcur%/%pllen%, %kfreq%kHz@%kbitrate%kbps)",
    not_running = "Not runnning...",
    }
end

-- merge external settings with defaults
local settings = table.join (statusd.get_config("xmmsip"), statusd_xmmsip)

-- location of the infopipe
local xmmsinfopipe = "/tmp/xmms-info"

-- info-pipe is named by user and session
if settings.user ~= nil -- should not happen... but default should be okay anyway...
  then
    xmmsinfopipe = xmmsinfopipe .. "_" .. settings.user .. "." .. settings.xmms_session
end

-- mapping to InfoPipe-keys
local valueassoc = {
  xmmsip_pllen = "Tunes in playlist",
  xmmsip_plcur = "Currently playing",
  xmmsip_file = "File",
  xmmsip_title = "Title",
  xmmsip_usectime = "uSecTime",
  xmmsip_time = "Time",
  xmmsip_usecpos = "uSecPosition",
  xmmsip_pos = "Position",
  xmmsip_left = "left",
  xmmsip_usecleft = "usecleft",
  xmmsip_status = "Status",
  xmmsip_sstatus = "sstatus",
  xmmsip_bitrate = "Current bitrate",
  xmmsip_kbitrate = "kbitrate",
  xmmsip_freq = "Samping Frequency", -- will this ever get fixed ?
  xmmsip_kfreq = "kfreq",
  xmmsip_channels = "Channels",
  xmmsip_xmmsproto = "XMMS protocol version",
  xmmsip_infover = "InfoPipe Plugin version",
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
  local bitrate = it[valueassoc["xmmsip_bitrate"]]
  if bitrate ~= nil
    then
      it[valueassoc["xmmsip_kbitrate"]] = math.floor(bitrate / 1000)
  end
  local status = it[valueassoc["xmmsip_status"]]
  if status ~= nil
    then
      it[valueassoc["xmmsip_sstatus"]] = string.lower(status)
  end
  local freq = it[valueassoc["xmmsip_freq"]]
  if freq ~= nil
    then
      it[valueassoc["xmmsip_kfreq"]] = math.floor(freq / 1000)
  end
  local usectime = it[valueassoc["xmmsip_usectime"]]
  local usecpos = it[valueassoc["xmmsip_usecpos"]]
  -- streaming leaves time at zero
  if ( ( usectime ~= nil ) and  ( usecpos ~= nil ) )
    then
      -- force lua to coerce this to a number...
      if ((usectime + 0) > 0)
        then
	  -- not streaming
          local usecleft = usectime - usecpos
          local mins = math.floor( usecleft / (60 * 1000) )
          local secs = math.floor( math.mod( usecleft, 60 * 1000 ) / 1000 )
          local left = string.format("%i:%02i", mins, secs )
          it[valueassoc["xmmsip_usecleft"]] = usecleft
          it[valueassoc["xmmsip_left"]] = left
	else
	  -- streaming
	  -- I would so like to use unicode for the
	  -- moebius but ion does not like it - yet...
	  it[valueassoc["xmmsip_time"]]=">>>"
	  it[valueassoc["xmmsip_left"]]="<<<"
	  it[valueassoc["xmmsip_usecleft"]]=0
    end
  end
  return it
end

-- this formats the user_format-string
local function makeUserString(s, it)
  local rval = s
  for k, v in pairs(valueassoc)
    do
      -- it would be annoying to type 'xmmsip_' in front
      -- of everything...
      _, _, stripped = string.find(k, "xmmsip_(.*)")
      rval = string.gsub(rval, "%%"..stripped.."%%", it[v])
  end
  return rval
end

-- our update timer
local xmmsip_timer

-- retrive information from the pipe - if it exists,
-- calculate some additional values and clean the table
-- from nilS
-- After that update statusd-monitors if the users wishes
-- and create the user-string from the template
local function fetch_data(partial_data)
  local infotable = { } -- tabula rasa
  local pipedata = ""
  while partial_data
    do
      pipedata = pipedata .. partial_data
      partial_data = coroutine.yield()
    end
    
  local running = true -- assume the best
  
  if pipedata and pipedata ~= "" 
    then
      for attribute, value in string.gfind(pipedata, "([^:]*):%s*([^\n]*)\n")
        do
	  infotable[attribute] = value
	end
    else
      -- obvious...
      running = false
      infotable[valueassoc["xmmsip_status"]]="Not running"
  end
   
  -- compute things like time left or divide things by 1000...
  infotable = addSpecialValues(infotable)
  -- scan for nil-values and fix them with '?'
  infotable = fixTable(infotable)
  
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
  if running
    then
      statusd.inform("xmmsip_user", makeUserString(settings.user_format, infotable))
    else
      statusd.inform("xmmsip_user", settings.not_running)
  end
  
  xmmsip_timer:set(settings.interval, update_xmmsip)
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
  -- This would be called a lot when the pipe is not
  -- running...
end

-- update monitor with bgread
function update_xmmsip()
  statusd.popen_bgread(
      'cat ' .. xmmsinfopipe
    , coroutine.wrap(fetch_data)
    , coroutine.wrap(flush_stderr)
  )
end

-- intialize timer
xmmsip_timer = statusd.create_timer()
-- start updating
update_xmmsip()

