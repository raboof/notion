-- Authors: pinko
-- License: Unknown
-- Last Changed: 2005-11-09
--
-- cfg_dock2.lua (pinko version)
--
-- An improved dock module configuration.
-- Remove the "2" and save to ~/.ion3, replacing any current
-- cfg_dock.lua. The script couldn't be put in a separate file
-- because it affects how the dock is created.
--
-- Features:
--
--  * key bindings for all functions that were in the button3 menu.
--  * switch between floating and embedded mode.
--	(Doing this restarts ion, so don't use with config files open.)
--  * button3 menu includes missing positions and ajusts to dock mode.
--  * flip or transpose the dock.
--  * bindings to move a floating dock by keyboard.
--  * mouse drag on border moves floating dock.
--  * move the dock anywhere, not just the sides and corners.
--  * attach current window to dock.
--  * support multiple docks if there are multiple screens.
--
--  Screenshot: http://silenceisdefeat.org/~chv/pics/screenshots/2docks.png
--
--
-- META+K D E   toggle between floating and embedded modes
-- META+K D F	flip
-- META+K D T	transpose
-- META+K D R	"resize mode" (it will move but not resize)
-- META+K D A	attach clientwin to dock
-- Button1	flip
-- Button2	transpose
-- ___________
-- |7   8   9|  META+K D (number) to move a floating dock.
-- |         |  Same for embedded dock, limited to 1,3,7,9.
-- |4   5   6|
-- |         |
-- |1   2   3|
-- -----------
--
-- 2005-11-09:
--  added this header.
--  changed name from "dock" to "dockN", N being a screen id.
--  flipped numbered bindings upside down to match keypad.
--  multiple screen support
--  arbitrary x,y position survives ion restart.


-- List IDs of all screens on which a dock should be created:
-- i.e., {0,1}, {0,2,4}
local use_these_screens={0}

docktable=ioncore.read_savefile("dock_settings")

if docktable==nil then
    docktable={}
end

function get_dock(scr)
    for i in pairs(scr:llist(2)) do
	if obj_is(scr:llist(2)[i],"WDock") then
	    return scr:llist(2)[i]
	end
    end
    for i in pairs(ioncore.region_list("WDock")) do
	if scr==ioncore.region_list("WDock")[i]:screen_of() then
	    return ioncore.region_list("WDock")[i]
	end
    end
end

function reset_dockgeom(scr)
    ioncore.defer(function()
	get_dock(scr):rqgeom({x=docktable[scr:id()].x, y=docktable[scr:id()].y})
    end)
end

for i in pairs(use_these_screens) do
    scr=use_these_screens[1]

    io.stdout:write(i)
    io.stdout:write(scr)
    if docktable[scr]==nil then
        docktable[scr] = {
	    screen=0,
	    mode="floating",
	    pos="tl",
	    grow="down",
	    floating_hidden=false,
	    x=0, y=0
        }
    end

    -- Create a dock based on a saved settings file.
    curdock=mod_dock.create{
        -- Dock mode: embedded|floating
        mode=docktable[scr].mode,
        -- The screen to create the dock on
        screen=scr,
        -- Corner or side of the screen to place the dock on.
        -- For embedded dock the valid values are: tl|tr|bl|br
        -- For floating dock the following are also valid: tc|bc|ml|mc|mr
        pos=docktable[scr].pos,
        -- Growth direction: left|right|up|down
        grow=docktable[scr].grow,
        -- Whether new dockapps should be added automatically to this dock
        is_auto=true,
        -- Show floating dock initially?
        floating_hidden=docktable[scr].floating_hidden,
        -- Name of the dock
        name="dock"..scr,
    }

    reset_dockgeom(ioncore.find_screen_id(scr))
end
ioncore.write_savefile("dock_settings", docktable)

defbindings("WScreen", {

    bdoc("Toggle floating dock."),
    kpress(META.."D", "mod_dock.set_floating_shown_on(_, 'toggle')"),
})

defbindings("WMPlex", {
    submap(META.."K", {
	submap("D", {
	    kpress("A", "get_dock(_:screen_of()):attach(_sub)", "_sub:WClientWin"),
	}),
    }),
})

defbindings("WScreen", {
    submap(META.."K", {
	submap("D", {
	    kpress("KP_7", "get_dock(_):set{pos='tl'}"),
	    kpress("KP_8", "get_dock(_):set{pos='tc'}"),
	    kpress("KP_9", "get_dock(_):set{pos='tr'}"),
	    kpress("KP_4", "get_dock(_):set{pos='ml'}"),
	    kpress("KP_5", "get_dock(_):set{pos='mc'}"),
	    kpress("KP_6", "get_dock(_):set{pos='mr'}"),
	    kpress("KP_1", "get_dock(_):set{pos='bl'}"),
	    kpress("KP_2", "get_dock(_):set{pos='bc'}"),
	    kpress("KP_3", "get_dock(_):set{pos='br'}"),
	    kpress("7", "get_dock(_):set{pos='tl'}"),
	    kpress("8", "get_dock(_):set{pos='tc'}"),
	    kpress("9", "get_dock(_):set{pos='tr'}"),
	    kpress("4", "get_dock(_):set{pos='ml'}"),
	    kpress("5", "get_dock(_):set{pos='mc'}"),
	    kpress("6", "get_dock(_):set{pos='mr'}"),
	    kpress("1", "get_dock(_):set{pos='bl'}"),
	    kpress("2", "get_dock(_):set{pos='bc'}"),
	    kpress("3", "get_dock(_):set{pos='br'}"),
	
	    kpress("R", "get_dock(_):begin_kbresize()"),
	    kpress("F", "flip_dock_direction(get_dock(_))"),
	    kpress("T", "cycle_dock_direction(get_dock(_))"),

	    kpress("E", "swap_dockmodes(_)"),

	    kpress("G", "reset_dockgeom(_:screen_of())"),
	}),
    }),
})


-- Dock settings menu. For this to work, mod_menu must have been loaded
-- previously.
if mod_menu then
    function dock_settings_menu(dock)
        if docktable[dock:screen_of():id()].mode=="floating" then
	    return {
		menuentry("Hide dock", "mod_dock.set_floating_shown_on("..
    	    	"_:screen_of(), 'toggle')"),
                menuentry("Pos-TL", "_:set{pos='tl'}"),
                menuentry("Pos-TC", "_:set{pos='tc'}"),
                menuentry("Pos-TR", "_:set{pos='tr'}"),
                menuentry("Pos-ML", "_:set{pos='ml'}"),
                menuentry("Pos-MC", "_:set{pos='mc'}"),
                menuentry("Pos-MR", "_:set{pos='mr'}"),
                menuentry("Pos-BL", "_:set{pos='bl'}"),
                menuentry("Pos-BC", "_:set{pos='bc'}"),
                menuentry("Pos-BR", "_:set{pos='br'}"),

                menuentry("Grow-L", "_:set{grow='left'}"),
                menuentry("Grow-R", "_:set{grow='right'}"),
                menuentry("Grow-U", "_:set{grow='up'}"),
                menuentry("Grow-D", "_:set{grow='down'}"),
        	menuentry("Embed dock", "swap_dockmodes(_:screen_of())"),
            }
        else
	   return {
                menuentry("Pos-TL", "_:set{pos='tl'}"),
                menuentry("Pos-TR", "_:set{pos='tr'}"),
                menuentry("Pos-BL", "_:set{pos='bl'}"),
                menuentry("Pos-BR", "_:set{pos='br'}"),

                menuentry("Grow-L", "_:set{grow='left'}"),
                menuentry("Grow-R", "_:set{grow='right'}"),
                menuentry("Grow-U", "_:set{grow='up'}"),
                menuentry("Grow-D", "_:set{grow='down'}"),
        	menuentry("Float dock", "swap_dockmodes(_:screen_of())"),
	   }
        end
    end
defbindings("WDock", {
    mpress("Button3", "mod_menu.pmenu(_, _sub, dock_settings_menu(_))"),
})
end


-- To use any of the dock's mouse bindings,
-- you'll have to find a bit of dead space for clicking in.
-- Near the border should always work.
-- (This is even necessary for Mod..Button bindings.)
defbindings("WDock", {
    mpress("Button2", "cycle_dock_direction(_)"),
    mclick("Button1", "flip_dock_direction(_)"),

    -- You can actually drag a floating dock anywhere.
    -- Position will be lost when ion restarts.
    mdrag("Button1", "_:p_move()"),
    mdrag(META.."Button1", "_:p_move()"),
})

function flip_dock_direction(dock)
    save_dock_settings()
    local dockgrow = dock:get().grow
    if dockgrow=="left" then
    	dock:set{grow='right'}
    elseif dockgrow=="right" then
    	dock:set{grow='left'}
    elseif dockgrow=="up" then
    	dock:set{grow='down'}
    elseif dockgrow=="down" then
    	dock:set{grow='up'}
    end
    reset_dockgeom(dock:screen_of())
end

function cycle_dock_direction(dock)
    save_dock_settings()
    local dockget = dock:get()
    local dockgeom = dock:geom()
    local screengeom = dock:screen_of():geom()

    if dockget.grow=="left" then
    	dock:set{grow='up'}
	if dockgeom.w + dockgeom.y > screengeom.h then
	    dock:rqgeom({x=dockgeom.x, y=screengeom.h - dockgeom.w})
	else
	    reset_dockgeom(dock:screen_of())
	end
    elseif dockget.grow=="right" then
    	dock:set{grow='down'}
	if dockgeom.w + dockgeom.y > screengeom.h then
	    dock:rqgeom({x=dockgeom.x, y=screengeom.h - dockgeom.w})
	else
	    reset_dockgeom(dock:screen_of())
	end
    elseif dockget.grow=="up" then
    	dock:set{grow='right'}
	if dockgeom.x + dockgeom.h > screengeom.w then
	    dock:rqgeom({x=screengeom.w - dockgeom.h, y=dockgeom.y})
	else
	    reset_dockgeom(dock:screen_of())
	end
    elseif dockget.grow=="down" then
    	dock:set{grow='left'}
	if dockgeom.x + dockgeom.h > screengeom.w then
	    dock:rqgeom({x=screengeom.w - dockgeom.h, y=dockgeom.y})
	else
	    reset_dockgeom(dock:screen_of())
	end
    end
end

function save_dock_settings()
    local dockget, dockgeom
    --docktable={}
    for i in pairs(ioncore.region_list('WDock')) do
	dock=ioncore.region_list('WDock')[i]
	dockget = dock:get()
	dockgeom = dock:geom()
	scr = dock:screen_of():id()
	if not docktable[scr] then docktable[scr]={} end
        docktable[scr].screen = scr
        docktable[scr].x = dockgeom.x
        docktable[scr].y = dockgeom.y
        docktable[scr].grow = dockget.grow
        docktable[scr].floating_hidden = dock:screen_of():l2_is_hidden(dock)
        if docktable[scr].mode=="embedded" then
	    docktable[scr].pos=string.gsub(string.gsub(dockget.pos, "m", "t"), "c", "l")
        else
            docktable[scr].pos = dockget.pos
        end
    end
    ioncore.write_savefile("dock_settings", docktable)
end

function swap_dockmodes(scr)
    if docktable[scr:id()].mode=="embedded" then
    	docktable[scr:id()].mode="floating"
    else
    	docktable[scr:id()].mode="embedded"
    end
    save_dock_settings()
    ioncore.restart()
end


--
-- This makes sure dock settings are saved any time ion restarts.
--
ioncore.get_hook("ioncore_deinit_hook"):add(
    function() save_dock_settings() end
)
