-- xinerama_switcher.lua
-- 
-- Goes to the frame in the specified direction. If there is no frame in the
-- given direction (the user is trying to move off the edge of the screen), it
-- switches workspace or screen. I assume that left/right switch between
-- screens, since xinerama displays are usually next to each other. Up/down,
-- on the other hand switch between workspaces. For me, the next workspace is
-- down as if I was reading a webpage.
--
-- The following keybindings may be useful:
--
--defbindings("WIonWS", {
--    kpress(MOD1.."Up", "xinerama_switcher_up(_)"),
--    kpress(MOD1.."Down", "xinerama_switcher_down(_)"),
--    kpress(MOD1.."Right", "xinerama_switcher_right(_)"),
--    kpress(MOD1.."Left", "xinerama_switcher_left(_)"),
--})
--
-- Based on go_desk_or_desk lua by Rene van Bevern <rvb@pro-linux.de>, 2005
--
-- This script is (c) copyright 2005 by martin f. krafft <madduck@madduck.net>
-- and released under the terms of the Artistic Licence.
--
-- Version 2005.08.14.1515
-- 
-- TODO: prevent switching screens when there is none on that side. E.g.
-- moving off the left edge of the left screen should either do nothing or
-- wrap to the right side of the right screen. This likely needs
-- a configuration file which would also solve the problem about assigning
-- left/right to next/prev according to xinerama configuration.
-- When solving this, don't just solve it for dual-head. :)
--

function goto_right_screen()
  -- do whatever it takes to switch to the screen on the right
  -- this may be either the previous or next screen, depending on how you set
  -- up xinerama.
  
  return ioncore.goto_prev_screen()
  --return ioncore.goto_next_screen()
end

function goto_left_screen()
  -- do whatever it takes to switch to the screen on the right
  -- this may be either the previous or next screen, depending on how you set
  -- up xinerama.

  --return ioncore.goto_prev_screen()
  return ioncore.goto_next_screen()
end

function goto_workspace_down(screen)
  -- do whatever it takes to switch to the workspace below
  -- in my book, that's the next workspace
  screen:switch_next()
end

function goto_workspace_up(screen)
  -- do whatever it takes to switch to the workspace below
  -- in my book, that's the previous workspace
  screen:switch_prev()
end
  
function xinerama_switcher(workspace, direction)

  if workspace:nextto(workspace:current(), direction, false) then
    -- there is a region in the sought direction, so just go to it
    workspace:goto_dir(direction)
  else
    -- the user is trying to move off the edge of the screen

    if direction == "left" then
      -- we move to the screen on the left
      local screen = goto_left_screen()
      screen:current():farthest("right"):goto()
    
    elseif direction == "right" then
      -- we move to the screen on the right
      local screen = goto_right_screen()
      screen:current():farthest("left"):goto()
    
    elseif direction == "up" then
      -- we switch to the workspace above this one
      local screen = workspace:screen_of()
      goto_workspace_up(screen)
      screen:current():farthest("down"):goto()
    
    elseif direction == "down" then
      -- we switch to the workspace below this one
      local screen = workspace:screen_of()
      goto_workspace_down(screen)
      screen:current():farthest("up"):goto()
      
    end
  end
end

-- helper functions for easier keybindings (see top of the file)

function xinerama_switcher_left(reg)
   xinerama_switcher(reg, "left")
end

function xinerama_switcher_right(reg)
   xinerama_switcher(reg, "right")
end

function xinerama_switcher_up(reg)
   xinerama_switcher(reg, "up")
end

function xinerama_switcher_down(reg)
   xinerama_switcher(reg, "down")
end
