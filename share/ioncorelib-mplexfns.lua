--
-- ion/share/ioncorelib-mplexfns.lua -- Misc. functions for WMPlex:s
-- 
-- Copyright (c) Tuomo Valkonen 2003.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--


-- Callback creation {{{

--DOC
-- Create a \type{WMPlex}-bindable function that calls \var{fn} with
-- parameter either the \type{WMPlex}, the current input in it or 
-- currently shown or some other object multiplexed in it dependent 
-- on the passed settings and parameters to the wrapper receives.
-- \var{noself}: Never call with the mplex itself as parameter.
-- \var{noinput}: Never call with current input as parameter.
-- \var{cwincheck}: Only call \var{fn} if the object selected is
-- a \type{WClientWin}.
function make_mplex_sub_or_self_fn(fn, noself, noinput, cwincheck)
    if not fn then
        warn("nil parameter to make_mplex_sub_fn")
    end
    return function(mplex, current)
               if not noinput then
                   local ci=mplex:current_input()
                   if ci then
                       current=ci
                   end
               end
               if not current then 
                   current=mplex:current()
               end
               if not current then
                   if noself then
                       return 
                   end
                   current=mplex
               end
               if not cwincheck or obj_is(current, "WClientWin") then
                   fn(current)
               end
               
           end
end

--DOC
-- Equivalent to \fnref{make_mplex_sub_or_self_fn}\code{(fn, true, true, false)}.
function make_mplex_sub_fn(fn)
    return make_mplex_sub_or_self_fn(fn, true, true, false)
end


--DOC
-- Equivalent to \fnref{make_mplex_sub_or_self_fn}\code{(fn, true, true, true)}.
function make_mplex_clientwin_fn(fn)
    return make_mplex_sub_or_self_fn(fn, true, true, true)
end

-- Backwards compatibility.
make_current_fn=make_mplex_sub_fn
make_current_clientwin_fn=make_mplex_clientwin_fn

-- }}}


-- Managed object indexing {{{

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

-- }}}
