-- Nest workspaces inside Frames.
-- Matthieu Moy <Matthieu.Moy@imag.fr>, February 15th 2005.
-- Made into a function (and to only show valid workspace types) by Etan Reisner <deryni@gmail.com> July 03 2006.
-- Public domain.

-- This adds a submenu to the client context menu.
-- Usage:
--        Add defbindings("WFrame", {
--            kpress(META.."M", "nest_ws_menu(_, 'Context menu: ', 'Attach')"), -- Query menu
--        })
--
--        or kpress(META.."M", "nest_ws_menu(_, _sub, 'Attach')"), -- Popup menu
--
-- This will override the default binding for the context menu and provide you
-- with a context menu with the nest_ws submenu.

-- LEGACY USAGE.
--
-- This defines a menu to be used as a submenu for WFrames.
-- Add the line
--       submenu("Attach",           "menuattach"),
-- to the definition defctxmenu("WFrame", { ... })

function nest_ws_menu(frame, cwin, name)
    if mod_menu then
	local framemenu = ioncore.getmenu("ctxmenu")(frame, cwin)
	local nestmenu = {}

	if mod_ionws then
	    table.insert(nestmenu, menuentry("WIonWS",   "_:attach_new({type=\"WIonWS\"  }):goto()"))
	end

	if mod_floatws then
	    table.insert(nestmenu, menuentry("WFloatWS", "_:attach_new({type=\"WFloatWS\"}):goto()"))
	end

	if mod_panews then
	    table.insert(nestmenu, menuentry("WPaneWS",  "_:attach_new({type=\"WPaneWS\" }):goto()"))
	end

	if #nestmenu > 0 then
	    local name = name or 'Attach'
	    table.insert(framemenu, submenu(name, nestmenu))
	end

	if (type(cwin) == "userdata" and obj_typename(cwin) == "WClientWin") or
	    not cwin then
	    mod_menu.menu(frame, cwin, framemenu)
	else
	    local prompt = cwin
	    mod_query.query_menu(frame, framemenu, prompt)
	end
    end
end

-- Legacy
defmenu("menuattach", {
    menuentry("WIonWS",   "_:attach_new({type=\"WIonWS\"  }):goto()"),
    menuentry("WFloatWS", "_:attach_new({type=\"WFloatWS\"}):goto()"),
    menuentry("WPaneWS",  "_:attach_new({type=\"WPaneWS\" }):goto()"),
})
