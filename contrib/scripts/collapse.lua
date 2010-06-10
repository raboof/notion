-- Move all windows on a WTiling to a single frame and destroy the rest.
-- (like C-x 1 in Emacs)
-- This is the ion3 version.

collapse={}

function collapse.take_frame_to_here (region, current)
   if region ~= current then
      region:managed_i(function (cwin)
                          ioncore.defer(function ()
                                           current:attach(cwin)
                                        end)
                          return true
                       end)
      ioncore.defer(function ()
                       region:rqclose()
                    end)
   end
   return true                   
end

function collapse.collapse(ws)
   local current = ws:current()
   ws:managed_i (function (region)
                    return collapse.take_frame_to_here(region, current)
                 end)
   current:goto()
end
