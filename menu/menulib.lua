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


--DOC
-- Use this function to define normal menu entries.
function menuentry(name, fn)
    return {name=name, fn=fn}
end


--DOC
-- Use this function to define menu entries for submenus.
function submenu(name, table)
    return {name=name, submenu=table}
end


--DOC
-- This function can be used to create a wrapper to create an
-- embedded-in-an-mplex menu and to call the handler for a menu
-- entry once selected. 
-- 
-- See also: \fnref{menu_menu}, \fnref{make_bigmenu_fn} and 
-- \fnref{make_pmenu_fn}.
function make_menu_fn(entries, big)
    return function(mplex, ...)
               local params=arg
               local function wrapper(entry)
                   if entry.fn then
                       entry.fn(mplex, unpack(params))
                   end
               end
               return menu_menu(mplex, wrapper, entries, big)
           end
end


--DOC
-- This function is similar to \fnref{make_menu_fn} but uses a possibly
-- bigger style for the menu.
function make_bigmenu_fn(entries)
    return make_menu_fn(entries, true)
end


--DOC
-- This function can be used to create a wrapper display a drop-down menu
-- and to call the handler for a menu entry once selected. The resulting
-- function should only be called from a mouse press binding.
-- 
-- See also: \fnref{menu_pmenu}, \fnref{make_menu_fn}.
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


--DOC
-- This function can be used to wrap the function \var{fn} so that
-- in frame context menus \var{fn} is correctly called either with
-- the frame, current managed object (e.g. client window) or in case
-- of drop-down menus the object corresponding to the tab that was 
-- clicked on. For example
-- \begin{verbatim}
--    menuentry("Close", make_menu_ctx_fn(WRegion.close)),
-- \end{verbatim}
-- will either close the frame or some client window correctly.
function make_menu_ctx_fn(fn)
    return function(frame, cwin)
               if not cwin or frame==cwin then
                   cwin=frame:current()
               end
               if cwin then 
                   fn(cwin) 
               end
           end
end
