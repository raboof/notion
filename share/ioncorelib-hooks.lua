--
-- ion/share/ioncorelib-hooks.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local ioncorelib=_G.ioncorelib

local hooks={}

--DOC
-- Call hook \var{hookname} with the remaining arguments to this function
-- as parameters.
function ioncorelib.call_hook(hookname, ...)
    if hooks[hookname] then
        for _, fn in hooks[hookname] do
            fn(unpack(arg))
        end
    end
end

--DOC
-- Register \var{fn} as a handler for hook \var{hookname}.
function ioncorelib.add_to_hook(hookname, fn)
    if not hooks[hookname] then
        hooks[hookname]={}
    end
    hooks[hookname][fn]=fn
end

--DOC
-- Unregister \var{fn} as a handler for hook \var{hookname}.
function ioncorelib.remove_from_hook(hookname, fn)
    if hooks[hookname] then
        hooks[hookname][fn]=nil
    end
end

