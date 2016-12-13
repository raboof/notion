-- Authors: Antti Vähäkotamäki
-- License: LGPL, version 2.1 or later
-- Last Changed: 2005
--
-- statusbar_external.lua
--

if not statusbar_external then
  statusbar_external = {

    -- a simple example which shows utc time and date
    -- in %udate every second with 'date -u'
    {
        template_key = 'dateb',
        external_program = 'date',
        execute_delay = 1 * 1000,
    },

    {
        template_key = 'udate',
        external_program = 'date -u',
        execute_delay = 5 * 1000,
    },

    -- a more complicated example showing the usage
    -- percentage of hda1 every 30 seconds in %hda1u
    -- with df, grep and sed.
    {
        template_key = 'hda1u',
        external_program = 'df|grep hda1|sed "s/.*\\(..%\\).*/\\1/"',
        execute_delay = 30 * 1000,
        meter_length = 4,
        hint_regexp = {
            important = '9[456].',
            critical = {
                '9[789].',
                '1...',
            },
        },
   },

  }
end

--
-- -- What is this?
--
-- This is a simple "general" statusbar lua script which
-- lets you show output of external programs in ion
-- statusbar. This is convenient if you are not familiar
-- with lua but can handle yourself with some other
-- language enough to output what you would want the
-- statusbar to show.
--
--
-- -- How to use it?
--
-- Short version:
--
-- 1 ) modify settings variable
-- 2 ) update template to include specified keys
-- 3 ) dopath( "statusbar_external" ) in cfg_user.lua
--
--
-- Longer version:
--
-- There is an example settings-variable which can be
-- modified to specify your own programs. The needed
-- parameters are the following :
--
-- * template_key
--  An unique name for the meter. You must add this
--  name with preceding % (for example %meter_name)
--  into your statusbar template which is usually set
--  in cfg_statusbar.lua.
--
-- * external_program
--  The program to execute. The last line of each
--  flushed output this program writes to stdout is
--  inserted as the content of the meter.
--
-- * execute_delay
--  The delay to wait before running the program again
--  once it stops sending data. Some programs just
--  send the data and exit while others continue
--  sending data periodically. This value can be used
--  to control the update rate for the first type of
--  programs.
--
-- * meter_length (optional)
--  The space reserved for this meter in characters.
--  If the value is not specified or is less than 1
--  then the space is determined by the length of the
--  data being shown but some people do not want their
--  statusbar width to change randomly.
--
-- * hint_regexp (optional)
--  These regular expressions (in Lua syntax) are
--  matched with the data and in case of a match the
--  named hint is set to the meter. The value can be
--  either a string containing the regexp or a table
--  containing several possible regexps (Lua regexp
--  doesn't have an 'or'). By default the themes define
--  a color for 'important' and 'critical' hints.
--
-- After this script is loaded it starts executing all
-- the provided programs and updates the statusbar every
-- time one of them prints (and flushes) something.
--
-- Also this is not a normal statusd script so it
-- doesn't get loaded by template name automatically
-- when ion starts. You must add
--
--   dopath( "statusbar_external" )
--
-- to some script that is being loaded in Ion. For
-- example ~/.ion3/cfg_user.lua is a good place. Note
-- that cfg_statusbar.lua is not loaded inside the
-- Ion process but in a separate process and thus this
-- script can not be loaded from that file!
--
--

---------------------------------------------------------

local timers = {}
local callbacks = {}

-- Just print out stderr for now.
local function flush_stderr(partial_data)
    local result = ""
    while partial_data do
        result = result .. partial_data
        -- Yield until we get more input.  popen_bgread will call resume on us.
        partial_data = coroutine.yield()
    end
    print(result, "\n")
end

-- Execute a program.  This will continuously retrieve output from the program.
function start_execute(key)
   -- Read output from the program and (if data) then update the statusbar
   local handle_output_changes = coroutine.create(
       function(input_data)
          local key = input_data
          local data = ""
          local partial_data = ""
          -- Keep reading data until we have it all
          while partial_data do
             data = data .. partial_data
             -- Yield until we get more input.
             -- popen_bgread will call resume on us.
             partial_data = coroutine.yield()
          end

          -- If no updates, then just schedule another run
          if not data or data == "" then
             return timers[key]:set(statusbar_external[key].execute_delay,
                                    callbacks[key])
          end

          -- remove all but the last line
          data = string.gsub( data, "\n$", "" )
          data = string.gsub( data, ".*\n", "" );

          local sets = statusbar_external[key]
          local t_key = sets.template_key
          local t_length = sets.meter_length
          local h_regexp = sets.hint_regexp

          if not tlength or t_length < 1 then
             t_length = string.len( data )
          end

          mod_statusbar.inform( t_key, data )
          mod_statusbar.inform( t_key .. "_hint", '' )
          mod_statusbar.inform(t_key .. "_template", string.rep( ' ', t_length))

          if not h_regexp then h_regexp = {} end

          for hint, re in pairs( h_regexp ) do
             if ( type( re ) == 'string' ) then
                re = { re }
             end

             for _, r in ipairs( re ) do
                if ( string.find( data, r ) ) then
                   mod_statusbar.inform(t_key .. "_hint", hint)
                end
             end
          end

          mod_statusbar.update()

          return timers[key]:set(statusbar_external[key].execute_delay,
                                 callbacks[key])
       end
    )

   -- We need to pass it the key before calling a background call.
   coroutine.resume(handle_output_changes, key)
   ioncore.popen_bgread(statusbar_external[key].external_program,
                        function(cor) coroutine.resume(handle_output_changes,
                                                       cor) end,
                        coroutine.wrap(flush_stderr))
end


-- Start all
for key in pairs(statusbar_external) do
    timers[key] = ioncore.create_timer()
	callbacks[key] = loadstring("start_execute("..key..")")
    start_execute(key)
end

--
-- Copyright (c) Antti Vähäkotamäki 2005.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--
