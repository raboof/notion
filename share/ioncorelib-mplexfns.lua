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
-- Create a \type{WMPlex}-bindable function with arguments
-- (\code{mplex [, sub]}) that calls \var{fn} with parameter chosen
-- as the first match from the following steps:
-- \begin{enumerate}
-- \item \var{sub} if it is not nil.
-- \item The current input in \var{mplex} (see \fnref{WMPlex.current_input})
--   if one exists and \var{noinput} is not set.
-- \item The currently displayed object in \var{mplex} 
--   (see \fnref{WMPlex.current}), if not nil.
-- \item \var{mplex} itself, if \var{noself} is not set.
-- \end{enumerate}
-- Additionally, if \var{cwincheck} is set, the function is only
-- called if the object selected above is of type \var{WClientWin}.
function make_mplex_sub_or_self_fn(fn, noself, noinput, cwincheck)
    if not fn then
        warn("nil parameter to make_mplex_sub_or_self_fn")
    end
    return function(mplex, tgt)
               if not tgt and not noinput then
                   tgt=mplex:current_input()
               end
               if not tgt then
                   tgt=mplex:current()
               end
               if not tgt then
                   if noself then
                       return 
                   end
                   tgt=mplex
               end
               if (not cwincheck) or obj_is(tgt, "WClientWin") then
                   fn(tgt)
               end
           end
end

--DOC
-- Create a \type{WMPlex}-bindable function with arguments
-- (\code{mplex [, sub]}) that calls \var{fn} with parameter chosen
-- as \var{sub} if it is not nil and otherwise \code{mplex:current()}.
-- 
-- Calling this functino is equivalent to
-- \fnref{make_mplex_sub_or_self_fn}\code{(fn, true, true, false)}.
function make_mplex_sub_fn(fn)
    return make_mplex_sub_or_self_fn(fn, true, true, false)
end


--DOC
-- Create a \type{WMPlex}-bindable function with arguments
-- (\code{mplex [, sub]}) that chooses parameter for \var{fn}
-- as \var{sub} if it is not nil and otherwise \code{mplex:current()},
-- and calls \var{fn} if the object chosen is of type \fnref{WClientWin}.
-- 
-- Calling this functino is equivalent to
-- \fnref{make_mplex_sub_or_self_fn}\code{(fn, true, true, true)}.
function make_mplex_clientwin_fn(fn)
    return make_mplex_sub_or_self_fn(fn, true, true, true)
end

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


-- {{{ Misc. functions

local function propagate_close(reg)
    if obj_is(reg, "WClientWin") then
        local l=reg:managed_list()
        if l then
            local r2=l[table.getn(l)]
            if r2 then
                return propagate_close(r2)
            end
        end
    end
    reg:close()
end                               

--DOC
-- Close a \type{WMPlex} or some child of it using \fnref{WRegion.close} as 
-- chosen by \fnref{make_mplex_sub_or_self}, but also, when the chosen object
-- is a \type{WClientWin}, propagate the close call to transients managed
-- by the \type{WClientWin}.
WMPlex.close_sub_or_self=make_mplex_sub_or_self_fn(propagate_close)

-- }}}
