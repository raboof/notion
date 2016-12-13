--

if package.loaded["mod_xrandr"] then return end

if not notioncore.load_module("mod_xrandr") then
    return
end

local mod_xrandr=_G["mod_xrandr"]

assert(mod_xrandr)

if not package.loaded["mod_xinerama"] then
    dopath("mod_xinerama")
end

function filter(t, predicate)
     local result = {}
     for k,v in pairs(t) do
         if predicate(v) then
	     result[k]=v
         end
     end
     return result
 end

function mod_xrandr.get_outputs(screen)
    -- get outputs based on geometry of this screen
    return mod_xrandr.get_outputs_for_geom(screen:geom())
end

function falls_within(geom)
    return function(output)
      local result = output.x >= geom.x and output.y <= geom.y
        and output.x + output.w <= geom.x + geom.w
        and output.y + output.h <= geom.y + geom.h
      return result;
    end
end

function mod_xrandr.get_outputs_within(all_outputs, screen)
    return filter(all_outputs, falls_within(screen:geom()))
end

-- Mark ourselves loaded.
package.loaded["mod_xrandr"]=true

-- Load configuration file
dopath("cfg_xrandr", true)
