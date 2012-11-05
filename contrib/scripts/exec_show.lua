-- Authors: Sadrul Habib Chowdhury <imadil@gmail.com>
-- License: Public domain
-- Last Changed: Unknown
-- 
-- exec_show.lua: executes a command and displays the result
--
-- this script is meant to be used when the output for the command
-- doesn't have more than a screenful of lines (approx 30-35 lines).
-- eg. tail, head, grep, ps ... etc.
-- 
-- CAUTION
-- DO NOT use this to execute something like xterm, vi or anything
-- similar (use the given key-bindings like MOD2..F2/F3/F4/F5 for those).
--
-- do let me know if this doesn't work for someone.
--
-- Author
-- Sadrul Habib Chowdhury (Adil)
-- imadil at gmail dot com

function show_result(mp, msg)
    mod_query.message(mp, msg)
end

function my_exec_handler(mp, cmd)
    -- not defer-ing was causing some probs
    ioncore.defer(
        function ()
            local f = io.popen(cmd, 'r')
            if not f then
                show_result(mp, 'error executing command: ' .. cmd)
                return
            end

            local s = f:read('*a')
            if s then 
                show_result(mp, s)
            else
                show_result(mp, 'no output')
            end
            f:close()   -- remove this, and watch your life get ruined (thanx Tuomo)
        end
    )

end

function exec_and_show(mplex)
    mod_query.query(mplex, TR("Exec and display:"), nil, my_exec_handler, 
            mod_query.exec_completor,
            "execdisplay")
end

defbindings("WMPlex", {
    bdoc("Execute a command and show the result."),
    kpress(MOD1.."F4", "exec_and_show(_)"),     -- change the binding to your liking
})

