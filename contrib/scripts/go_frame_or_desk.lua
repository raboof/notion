-- Goes to the frame in the specified direction. If there is no frame in the
-- given direction, it goes to the next workspace in the direction, being:
-- 	left  = previous workspace
-- 	right = next workspace
--
-- By Rene van Bevern <rvb@pro-linux.de>, 2005
-- 	Public Domain
--
-- If you are about to go to a frame that would be left to the leftmost frame,
-- the function switches to a previous workspace and goes to its rightmost frame.
-- If you are about to go to a frame that would be right of the rightmost frame,
-- the function switches to the next workspace and goes to its leftmost frame.
--
-- To use this function you need to bind keys for WIonWS, for exmaple:
--
--defbindings("WIonWS", {
--    kpress(MOD1.."Up", "go_frame_or_desk_up(_)"),
--    kpress(MOD1.."Down", "go_frame_or_desk_down(_)"),
--    kpress(MOD1.."Right", "go_frame_or_desk_right(_)"),
--    kpress(MOD1.."Left", "go_frame_or_desk_left(_)"),
--})

function go_frame_or_desk(workspace, direction)
   local region = workspace:current()
   local screen = workspace:screen_of()
   if workspace:nextto(region,direction,false) then
      workspace:goto_dir(direction)
   elseif direction == "left" then
      screen:switch_prev()
      screen:current():farthest("right"):goto()
   elseif direction == "right" then
      screen:switch_next()
      screen:current():farthest("left"):goto()
   end
end

function go_frame_or_desk_left(reg)
   go_frame_or_desk(reg, "left")
end

function go_frame_or_desk_right(reg)
   go_frame_or_desk(reg, "right")
end

function go_frame_or_desk_up(reg)
   go_frame_or_desk(reg, "up")
end

function go_frame_or_desk_down(reg)
   go_frame_or_desk(reg, "down")
end
