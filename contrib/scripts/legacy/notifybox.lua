-- Authors: Unknown
-- License: Unknown
-- Last Changed: Unknown
--
notifybox={}

local default="*notifybox*"

local function get_box(name)
    return ioncore.lookup_region(name, "WInfoWin")
end

function notifybox.show(message, name, location, style, scr)
    if not name then
        name=default
    end
    
    if not scr then
        scr=ioncore.find_screen_id(0)
    end
    
    local box=get_box(name)
    
    if not box then
        box=scr:attach_new{
            type="WInfoWin",
            name=name,
            sizepolicy=location or "southeast",
            unnumbered=true,
            level=10,
            passive=true,
            style=style,
            geom={w=1, h=1, x=0, y=0},
        }
    end

    -- Hack: attach_new doesn't get geometries right if we pass msg 
    -- directly to it.
    box:set_text(message, scr:geom().w)
end


function notifybox.hide(name)
    local box=get_box(name or default)
    
    if box then
        box:rqclose()
    end
end
