--
-- ion/share/ioncorelib-extras.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local ioncorelib=_G.ioncorelib

local defcmd=ioncorelib.defcmd
local defcmd2=ioncorelib.defcmd2

-- WRegion extras {{{

--DOC
-- Call parameter command on the screen of a region.
defcmd("WRegion", "@screen_of",
       function(reg, ...)
           local scr=reg:screen_of()
           if scr then
               ioncorelib.do_cmd_t(scr, nil, arg)
           end
       end)

--DOC
-- Call parameter command on the workspace of a region.
defcmd("WRegion", "@ws_of",
       function(reg, ...)
           local ws=ioncorelib.find_manager(reg, "WGenWS")
           if ws then
               ioncorelib.do_cmd_t(ws, nil, arg)
           end
       end)

-- }}}


-- WMPlex extras {{{

local function make_mplex_sub_or_self_cmd(fn, noself, noinput, cwincheck)
    if not fn then
        warn("nil parameter to make_mplex_sub_or_self_fn")
    end
    return function(mplex, tgt, ...)
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
                   fn(tgt, nil, arg)
               end
           end
end

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
-- Call parameter command on an object managed by the multiplexer,
-- or the multiplexer itself. The method of selecting the managed
-- object are as for \cmdref{WMPlex}{@sub}.
defcmd2("WMPlex", "@sub_or_self",
        make_mplex_sub_or_self_cmd(ioncorelib.do_cmd_t, false, false, false))

--DOC
-- Call parameter command on an object managed by the multiplexer.
-- The selected object is the second argument passed to this command 
-- or, if none is passed, the currently active managed object.
defcmd2("WMPlex", "@sub",
        make_mplex_sub_or_self_cmd(ioncorelib.do_cmd_t, true, true, false))

--DOC
-- Call parameter command on an a client window managed by the multiplexer.
-- The selected object is the second argument passed to this command 
-- or, if none is passed, the currently active object. If the object
-- selected is not of type \type{WClientWin}, no action is taken.
defcmd2("WMPlex", "@sub_cwin",
        make_mplex_sub_or_self_cmd(ioncorelib.do_cmd_t, true, true, true))

--DOC
-- Call parameter command on an object managed by the multiplexer.
-- The selected object is the second argument passed to this command 
-- or, if none is passed, the currently active input or multiplexed
-- object, in that order of preference.
defcmd2("WMPlex", "@sub_or_input",
        make_mplex_sub_or_self_cmd(ioncorelib.do_cmd_t, true, false, false))

--DOC
-- Close managed object or the multiplexer itself, if there is none.
defcmd2("WMPlex", "close_sub_or_self",
        make_mplex_sub_or_self_cmd(propagate_close))

--DOC
-- Move currently viewed object left within the multiplexer.
defcmd2("WMPlex", "inc_managed_index",
        function(mplex, r)
            if not r then
                r=mplex:current()
            end
            if r then
                mplex:set_mgd_index(r, mplex:get_mgd_index(r)+1) 
            end
        end)

--DOC
-- Move currently viewed object right within the multiplexer.
defcmd2("WMPlex", "dec_managed_index",
        function(mplex, r)
            if not r then
                r=mplex:current()
            end
            if r then
                mplex:set_mgd_index(r, mplex:get_mgd_index(r)-1) 
            end
        end)


-- }}}


-- WGenWS extras {{{

--DOC
-- Call argument command on the current object on workspace.
defcmd2("WGenWS", "@sub",
        function(ws, sub, ...)
            if not sub then
                if ws["current"] then
                    sub=ws:current()
                end
                if not sub then
                    return
                end
            end
            ioncorelib.do_cmd_t(sub, nil, arg)
        end)

-- }}}


-- WScreen extras {{{

--DOC
-- Create new workspace on screen.
defcmd("WScreen", "create_new_ws",
       function(scr, ws_type)
           scr:attach_new({
               type=(ws_type or DEFAULT_WS_TYPE),
               switchto=true
           })
       end)

--DOC
-- Close current workspace.
defcmd("WScreen", "close_current_ws",
       function(scr)
           local c=scr:current()
           if obj_is(c, "WGenWS") then 
               c:close() 
           end
       end)

-- }}}


-- Miscellaneous {{{

--DOC
-- Find an object with type name \var{t} managing \var{obj} or one of
-- its managers.
function ioncorelib.find_manager(obj, t)
    while obj~=nil do
        if obj_is(obj, t) then
            return obj
        end
        obj=obj:manager()
    end
end

--DOC
-- Export a list of functions from \var{lib} into global namespace.
function ioncorelib.export(lib, ...)
    for k, v in arg do
        _G[v]=lib[v]
    end
end

--DOC
-- Show ion manual.
defcmd("global", "show_manual",
       function(for_what)
           local script=lookup_script("ion-man")
           if script then
               exec(script.." "..(for_what or "ion"))
           end
       end)

-- }}}

