--
-- This script allows making panels and other windows the stdisp.
--
-- Usage:
--
-- defwinprop { 
--      class = 'SomePanel',
--      instance = 'somepanel',
--      is_panel = true,
--      -- panel_fullsize = false,
--      -- panel_pos = 'bl',
--      -- min_size = { w = something, h = sensible },
-- }
--
-- Set panel.check_exists to disable checking for existing stdisp.
--

panel={}

panel.check_exists=true

local function dflt(x, y)
    if x~=nil then return x else return y end
end

function panel.manage(cwin, tab)
    local prop=ioncore.getwinprop(cwin)
    if prop and prop.is_panel then
        local scr=cwin:screen_of()
        local exists=false
        
        if panel.check_exists then
            local current=scr:get_stdisp()
            if current and current.reg then
                exists=true
            end
        end
        
        if not exists then
            return scr:set_stdisp{
                reg = cwin,
                fullsize = dflt(prop.panel_fullsize, true),
                pos = dflt(prop.panel_pos, 'bl'),
            }
        end
    end
    return false
end


function panel.reg()
    ioncore.get_hook("clientwin_do_manage_alt"):add(panel.manage)
end

function panel.unreg()
    ioncore.get_hook("clientwin_do_manage_alt"):remove(panel.manage)
end
    
panel.reg()

    