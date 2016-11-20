-- Authors: Hendrik Iben <hiben@tzi.de>
-- License: GPL, version 2
-- Last Changed: 2006-03-23
--
-- statusd_fortune.lua v20060323
--
-- Displays a fortune (in a sane way for one line).
-- My first attempt at writing in lua and for ion.
--
-- Written by Hendrik Iben < hiben at tzi dot de >
--
-- How to use :
-- fortune provides a monitor for statusd.
-- This monitor is feed with a fortune obtained from the famous fortune-
-- program. But as fortunes can span several lines and the ion-statusbar
-- only provides one line there is some conversion done.
--
-- As a user you can influence the way this conversion is done by specifying
-- with what string empty space and newlines are to be replaced. Or just
-- leave the defaults.
--
-- For a start you can simple add '%fortune' to your statusbar-template.
--
-- Requirements:
-- ion3 (tested with 20060305 gentoo ebuild)
-- lua (tested with 5.0.2)
-- fortune (any version will do) or anything that returns text :-)
--
-- Options :
-- The settings you can specify (and their defaults) are :
-- interval     : minimum time between fortune updates (60 seconds)
-- minwait      : additional time to always wait for a fortune (6 seconds)
-- cpers        : number of characters to increase waiting by one second (20)
-- command      : command to obtain a fortune (fortune -s)
-- prefix       : head of every fortune
-- postfix      : tail of every fortune
-- spacereplace : string that replaces empty space of more than 3 characters
--                (' - ')
-- newline      : string that replaces newlines (' > ')
-- newlinetrick : try to be smart when killing newlines (true)
--
-- Setting 'interval' and 'minwait' together might seem useless but when
-- calculating  the waiting time the maximum of 'minwait' and the time
-- resulting from 'cpers' is added to 'interval'. The defaults for 'minwait'
-- and 'cpers' are taken from fortunes source.
--
-- The default command triggers a short fortune (see: man fortune) as long
-- fortunes tend to fill more than one line in the status-bar. You may wish
-- to enable offending fortunes but I personally don't like them that much...
-- In my opinion the statusbar should offer things like scrolling text but
-- eventually I will implement this in a later release of this monitor...
--
-- Enabling 'newlinetrick' basicly makes this program look at the ending and
-- beginning of the two lines and check if they are *meant* to be separated.
-- Currently this merges lines that end and begin with lower-case-letters and
-- silently removes newlines in front of 'cite-signs' (e.g.
-- "...blah\n-- Hendrik" becomes "...blah -- Hendrik").
--
-- Example configuration :
-- cfg_statusbar (rotate_statusbar that is)
--   ...
--   ...
--   rotate_statusbar.configurations = {
--     ...
--     ...
--      fortune = {
--        prefix = "Fortune : ",
--        interval = 30*1000,
--     },
--     ...
--     ...
--
--   rotate_statusbar.settings = {
--    ...
--    ...
--    all_statusbars = {
--      "[ %date || %xmmsip_user ]%filler%systray",
--      "[ %fortune ]%filler%systray",
--      ...
--      ...
--
-- This setup makes the basic fortune update all 30 seconds and prepends
-- every fortune with 'Fortune : '. Also using rotate_statusbar I decided
-- to give fortunes the full bar turning of the date-monitor so I don't
-- perceive how much time I spend on watching fortunes instead of doing
-- more creative things... :-)
--
-- Happy fortunes for you!
--
-- Feel free to contact me if you discover bugs or want to comment on this.
--  -- Hendrik
--
-- You are free to distribute this software under the terms of the GNU
-- General Public License Version 2.

if not statusd_fortune then
  statusd_fortune={
    interval=60*1000,
    minwait=6*1000,
    cpers=20,
    command="fortune -s",
    prefix="",
    postfix="",
    spacereplace=" - ",
    newline=" > ",
    newlinetrick=true,
  }
end

local settings = table.join (statusd.get_config("fortune"), statusd_fortune)

-- do things to strings...
-- did I mention I write Haskell normally ? :-)
local function flatspace(s)
  return
    string.gsub( -- 1. remaining controls
      string.gsub( -- 2. newlines not covered by newlinetrick
        string.gsub( -- 3. spaces
          string.gsub( -- 4. remove trailing control + space
            string.gsub(s, "^[%c%s]+", "") -- 5. remove leading control + space
            , "[%c%s]+$"
            , ""
          ) -- > 4.
          , "%s%s%s%s+"
          , settings.spacereplace
        ) -- > 3.
        , "\n+"
        , settings.newline
      ) -- > 2.
      , "%c"
      , ""
    ) -- > 1.
end

-- our timer
local fortune_timer

-- on error we get the message, but a bit to late...
local last_error = "error pending..."

-- receives data from settings.command output
local function receive_fortune_status(partial_data)
  local fortunestring = ""

  while partial_data
    do
      fortunestring = fortunestring .. partial_data
      partial_data = coroutine.yield()
    end

  if (not fortunestring) or (fortunestring == "")
    then
      -- we get here if e.g. the command was not found...
      fortunestring = "could not obtain a fortune! ("..last_error..")"
  end

  -- perform newline-merging
  if settings.newlinetrick
    then
      -- trick 1 : wrapped sentences
      fortunestring = string.gsub(fortunestring, "(%l)\n(%l)", "%1 %2")
      -- trick 2 : 'cite-signs'
      fortunestring = string.gsub(fortunestring, "([^\n]*)[%c%s]*(--.*)", "%1 %2")
  end

  -- flatspace is such a silly name...
  local f = flatspace(fortunestring)

  -- calculate time to wait
  local waittime =
    settings.interval
    + math.max(
       settings.minwait -- minimun
      ,1000 * (string.len(f) / settings.cpers) -- maybe more
    )

  -- feed the monitor
  statusd.inform("fortune", settings.prefix..f..settings.postfix)

  -- update callback
  fortune_timer:set(waittime, update_fortune)
end

local function flush_stderr(partial_data)
  local r = ""

  while partial_data
    do
      r = r .. partial_data
      partial_data = coroutine.yield()
    end
  -- update last error to give the user a hint...
  last_error = r
end

--- monitor-update
function update_fortune()
  statusd.popen_bgread(
      settings.command
    , coroutine.wrap(receive_fortune_status)
    , coroutine.wrap(flush_stderr)
    )
end

-- initialize our timer
fortune_timer = statusd.create_timer()
-- begin
update_fortune()
