--
-- ion/share/ioncore-mplexfns.lua -- Misc. functions for WMPlex:s
-- 
-- Copyright (c) Tuomo Valkonen 2003.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

if WMPlex.managed_index then
    -- Already loaded
    return
end

--DOC
-- Returns the index of \var{mgd} in \var{mplex}'s managed list or
-- -1 if not on list.
function WMPlex.managed_index(mplex, mgd)
    local list=mplex:managed_list()
    for idx, mgd2 in list do
        if mgd2==mgd then
            return idx
        end
    end
    return -1
end


--DOC
-- Returns the index of \fnref{WMPlex.current} in a \var{mplex}'s
-- managed list or -1 if there is no current managed object.
function WMPlex.current_index(mplex)
    return mplex:managed_index(mplex:current())
end

--DOC
-- Move currently viewed object left within mplex; same as
-- \code{mplex:move_left(mplex:current())}.
function WMPlex.move_current_to_next_index(mplex)
    local c=mplex:current()
    if c then mplex:move_to_next_index(c) end
end

--DOC
-- Move currently viewed object right within mplex; same as
-- \code{mplex:move_left(mplex:current())}.
function WMPlex.move_current_to_prev_index(mplex)
    local c=mplex:current()
    if c then mplex:move_to_prev_index(c) end
end
