-- 

if package.loaded["mod_xrandr"] then return end

if not ioncore.load_module("mod_xrandr") then
    return
end

local mod_xrandr=_G["mod_xrandr"]

assert(mod_xrandr)

if not package.loaded["mod_xinerama"] then 
    dopath("mod_xinerama")
end

function mod_xrandr.get_outputs(screen)
    -- get outputs based on geometry of this screen
    return mod_xrandr.get_outputs_for_geom(screen:geom())
end
