--
-- statusd_exec.lua
--
--
-- -- What is this?
--
-- This is a simple "general" statusd Lua script which
-- lets you show output of external programs in ion
-- statusbar. This is convenient if you are not familiar
-- with Lua but can handle yourself with some other
-- language enough to output what you would want the
-- statusbar to show.
--
-- After this script is loaded it starts executing all
-- the provided programs (wether a placeholder exists in
-- template or not) and updates the statusbar every time
-- one of them prints (and flushes) a line.
-- 
-- -- How to use it?
--
-- 1 ) update statusbar template to include %exec_something
-- 2 ) specify parameters in cfg_statusbar.lua like this:
--
-- mod_statusbar.launch_statusd{
--
--     [ ... some other config here ... ],
--
--     exec = {
--
--         -- show date each second in %exec_date with.. date!
-- 
--         date = {
--             program = 'date',
--             retry_delay = 1 * 1000,
--         },
--                  
--         -- show percentage of hda1 every 30 seconds in 
--         -- %exec_hda1u with df, grep and sed.
--         -- also highlights entry if usage is high.
--
--         hda1u = {
--             program = 'df|grep hda1|sed "s/.*\\(..%\\).*/\\1/"',
--             retry_delay = 30 * 1000,
--             meter_length = 4,
--             hint_regexp = {
--                 important = '9[456].',
--                 critical = {
--                     '9[789].',
--                     '1...',
--                 },
--             },
--         },
-- 
--         -- keep continous track of a remote machine in
--         -- %exec_remote with ssh, bash and uptime.
--         -- if the connection breaks, wait 60 seconds and retry
--         
--         remote = {
--             program = 'ssh -tt myhost.com "while (uptime);' ..
--                       'do sleep 1; done"',
--             retry_delay = 60 * 1000,
--         },
--     }
-- }
--
-- -- Description of variables:
-- 
-- * program
--  The program to execute. The last line of each
--  flushed output this program writes to stdout is
--  inserted as the content of the meter.
--
-- * retry_delay (optional)
--  The delay to wait (ms) before running the program again
--  once it stops sending data. Some programs just
--  send the data and exit while others continue
--  sending data periodically. This value can be used
--  to control the update rate for the first type of
--  programs. You can also specify a value for the other
--  type of programs to retry if the program stops for
--  some reason.
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
-- 
                
local settings = table.join( statusd.get_config("exec"), {} )
local start_execution = nil
local timers = {}

-- Function for starting and restarting execution of a process

start_execution = function ( key )

    -- Set up a function for receiving the data
    -- and informing the statusbar

    local process_input = function ( new_data )

        local received_data = ""
        
        -- When we get new data, output the last complete line
           
        while new_data do
            received_data = received_data .. new_data
          
            -- Erase old data which we do not need anymore
            
            received_data = string.gsub( received_data,
                                         ".*\n(.*\n.*)", "%1" )

            -- Set the last complete line as the current line

            local current_line = string.gsub( received_data,
                                              "(.*)\n.*", "%1" )
                                                    
            local t_key = "exec_" .. key
            local t_length = settings[key].meter_length
            local h_regexp = settings[key].hint_regexp

            if not tlength or t_length < 1 then
                t_length = string.len( current_line )
            end

            statusd.inform( t_key .. "_hint", '' )
            statusd.inform( t_key .. "_template",
                            string.rep( 'x', t_length ) )
            statusd.inform( t_key, current_line )

            if not h_regexp then h_regexp = {} end

            for hint, re in pairs( h_regexp ) do
                if ( type( re ) == 'string' ) then
                    re = { re }
                end
        
                for _, r in ipairs( re ) do
                    if ( string.find( current_line, r ) ) then
                       statusd.inform(t_key .. "_hint", hint)
                    end
                end
            end
            
            -- Wait for bgread to signal that more data
            -- is available or the program has exited
            
            new_data = coroutine.yield()
        end

        -- Program has exited.
        -- Start a timer for a new execution if set.
        
        if settings[key].retry_delay then
            timers[key]:set( settings[key].retry_delay,
                             function()
                                 start_execution(key)
                             end )
        end
    end

    -- Set up a simple function for printing errors

    local process_error = function( new_data )
        while new_data do
            io.stderr:write( "ion-statusd [statusd_exec, key='"
                .. key .. "']: " .. new_data )
            io.stderr:flush()
            new_data = coroutine.yield()
        end
    end

    -- Execute the program in the background and create
    -- coroutine functions to handle input and error data
    
    statusd.popen_bgread( settings[key].program,
                          coroutine.wrap( process_input ),
                          coroutine.wrap( process_error ) )

end

-- Now start execution of all defined processes

for key in pairs(settings) do
    timers[key] = statusd.create_timer()
    start_execution(key)
end

--
-- Copyright (c) Antti Vähäkotamäki 2006.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--
