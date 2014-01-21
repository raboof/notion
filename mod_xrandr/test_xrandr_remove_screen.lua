-- {{{ mock notion context
function iter(t, cb)
  for i,o in pairs(t) do
    cb(o);
  end
end

function move(ws, from, to)
  if from[1] == ws then 
    table.remove(from, 1)
    table.insert(to, ws)
  else
    assert(false);
  end
end

local workspaces1 = {}
local workspaces2 = {}
local firstscreen = {
    geom = function() return { x=0, y=0, w=1920, h=1080 } end,
    name = function() return "WScreen<1>" end,
    managed_i = function(screen, callback) iter(workspaces1, callback) end,
    attach = function(s, ws) move(ws, workspaces2, workspaces1); ws.screen = firstscreen end,
    rqclose = function() end,
    mx_count = function() return #workspaces1 end,
    id = function() return 0 end
}
local secondscreen = {
    geom = function() return { x=0, y=0, w=1920, h=1080 } end,
    name = function() return "WScreen" end,
    managed_i = function(screen, callback) iter(workspaces2, callback) end,
    attach = function(ws) move(ws, workspaces1, workspaces2); ws.screen = secondscreen end,
    rqclose = function() end,
    mx_count = function() return #workspaces2 end,
    id = function() return 1 end
}
local firstws = {
    screen_of = function() return firstscreen end,
    name = function() return "firstws" end
}
table.insert(workspaces1, firstws)

local secondws = {
    screen_of = function() return secondscreen end,
    name = function() return "secondws" end
}
table.insert(workspaces2, secondws)

function obj_is(o, t)
  if (o == firstws or o == secondws) and t == 'WGroupWS' then return true end
  return false
end

WGroupWS = {
  get_initial_outputs = function(ws) 
    if ws == firstws then return { 'VGA1' } end
    if ws == secondws then return { 'VGA1' } end
    return nil
  end
}

notioncore = { 
  rootwin = function() end,
  screens_updated = function(rootwin) end,
  load_module = function() return 1 end,
  get_hook = function() return { add = function() end; } end,
  region_i = function(callback, type)
      local next = callback(firstscreen)
      if next then callback(secondscreen) end
  end,
  find_screen_id = function(screen_id) 
    if screen_id == 0 then return firstscreen 
    else 
      if screen_id == 1 then return secondscreen
      else return nil 
        end
    end
  end
}

function dopath() 
end

-- all outputs reported by xrander. Notably, the second screen is (now) missing.
local all_outputs = { VGA1 = { x=0, y=0, w=1680, h=1050, name='VGA1' } }

_G["mod_xinerama"] = {
  update_screen = function() end,
  query_screens = function() return { { x=0, y=0, w=1680, h=1050 } } end
}

_G["mod_xrandr"] = {
    get_all_outputs = function()
            return all_outputs;
        end
}

-- }}}

-- initial
assert(workspaces2[1] ~= nil);


-- load xrandr module lua code
dofile('../mod_xinerama/mod_xinerama.lua')
dofile('mod_xrandr.lua')
dofile('cfg_xrandr.lua')

mod_xrandr.rearrangeworkspaces(0)
assert(nilOrEmpty(workspaces2));
