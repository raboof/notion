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

if mplex_managed_index then
    -- Already loaded
    return
end

--EXTL_EXPORT
-- Returns the index of \var{mgd} in \var{mplex}'s managed list or
-- -1 if not on list.
function mplex_managed_index(mplex, mgd)
    local list=mplex_managed_list(mplex)
    for idx, mgd2 in list do
        if mgd2==mgd then
            return idx
        end
    end
    return -1
end


--EXTL_EXPORT
-- Returns the index of \fnref{mplex_current}\code{(mplex)} in \var{mplex}'s
-- managed list or -1 if there is no current managed object.
function mplex_current_index(mplex)
    return mplex_managed_index(mplex, mplex_current(mplex))
end
