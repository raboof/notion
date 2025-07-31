-- Copyright (c) 2025 Matthias S. Benkmann
-- You may do everything with this code except misrepresent its origin.
-- PROVIDED `AS IS' WITH ABSOLUTELY NO WARRANTY OF ANY KIND!

-- Create submenus that are dynamically created by an external script whenever
-- they are opened. One use-case demonstrated by the script notionmount.pl
-- is a dynamic submenu that lists removable USB drives with their mounted/unmounted
-- status like this
--
--   [_] Drive label 1 (unmounted)
--   [X] Drive label 2 (mounted)
--
-- and allows mounting/unmounting them by selecting the respective menu entry.
--
-- The above submenu can be added to menu like this:
--   submenu("USB", function() return cmd2menu('notionmount.pl -menu') end),


-- Takes a command line cmd and executes it via the shell.
-- The command must output lines in the format
--     Menu entry label\0Notion command to execute
-- Each line that abides by this format will create a menu entry.
-- cmd2menu() returns a menu with all the generated entries.
--
-- E.g. a trivial shell script that would work for cmd2menu:
--   #!/bin/sh
--   echo "Start Gimp\0ioncore.exec_on(_, 'gimp')"
function cmd2menu(cmd)
  local menu = {}
  local f = io.popen(cmd, 'r')
  if not f then
    table.insert(menu, menuentry("Error", "nil"))
  else
    for line in f:lines() do
      local pos = string.find(line, "\0")
      if pos then
        local name = string.sub(line, 1, pos - 1)
        local menucmd = string.sub(line, pos + 1)
        table.insert(menu, menuentry(name, menucmd))
      end
    end
    f:close()
  end

  return menu
end
