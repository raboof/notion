--
-- ion/share/menulib.lua -- Helper functions for defining menus.
-- 
-- Copyright (c) Tuomo Valkonen 2003.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

function menuentry(name, fn)
    return {name=name, fn=fn}
end

function submenu(name, table)
    return {name=name, submenu=table}
end

function make_menu_fn(entries)
    return function(mplex, ...)
               local params=arg
               local function wrapper(entry)
                   if entry.fn then
                       entry.fn(mplex, unpack(params))
                   end
               end
               return menu_menu(mplex, wrapper, entries)
           end
end

function make_pmenu_fn(entries)
    return function(mplex, ...)
               local params=arg
               local function wrapper(entry)
                   if entry.fn then
                       entry.fn(mplex, unpack(params))
                   end
               end
               return menu_pmenu(mplex, wrapper, entries)
           end
end

function make_ctxmenu_fn(fn)
    return function(frame, cwin)
               if not cwin or frame==cwin then
                   cwin=frame:current()
               end
               if cwin then 
                   fn(cwin) 
               end
           end
end
