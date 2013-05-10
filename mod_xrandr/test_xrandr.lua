-- {{{ mock notion context
local firstscreen = {
    geom = function() return { x=0, y=0, w=1680, h=1050 } end,
    name = function() return "WScreen<1>" end,
    id = function() return 0 end
}
local secondscreen = {
    geom = function() return { x=1680, y=0, w=1920, h=1080 } end,
    name = function() return "WScreen" end,
    id = function() return 1 end
}

notioncore = { 
  load_module = function() return 1 end,
  get_hook = function() return { add = function() end; } end,
  region_i = function(callback, type)
      local next = callback(firstscreen)
      if next then callback(secondscreen) end
  end
}

function dopath() 
end

local all_outputs = { VGA1 = { x=0, y=0, w=1680, h=1050, name='VGA1' }, LVDS1 = { x=1680, y=0, w=1920, h=1080, name='LVDS1' } } 

_G["mod_xrandr"] = {
--  query_screens = function() end
    get_all_outputs = function()
            return all_outputs;
        end
}

-- }}}

-- load xrandr module lua code
dofile('mod_xrandr.lua')
dofile('cfg_xrandr.lua')

local falls_within_secondscreen = falls_within(secondscreen.geom())
assert(falls_within_secondscreen(secondscreen:geom()))
assert(falls_within_secondscreen(firstscreen:geom())==false)

local outputs_within_secondscreen = mod_xrandr.get_outputs_within(all_outputs, secondscreen)
assert(singleton(outputs_within_secondscreen))
assert(firstValue(outputs_within_secondscreen).y == 0)
assert(firstValue(outputs_within_secondscreen).x == 1680)

local candidates = candidate_screens_for_output(1, all_outputs, 'VGA1')
assert(singleton(candidates))
assert(firstKey(candidates) == "WScreen<1>")

candidates = candidate_screens_for_outputs(1, all_outputs, {'VGA1'})
assert(singleton(candidates))
assert(firstKey(candidates) == "WScreen<1>")

candidates = candidate_screens_for_output(1, all_outputs, 'LVDS1')
assert(singleton(candidates))
assert(firstKey(candidates) == "WScreen")
