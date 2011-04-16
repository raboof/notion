-- mock notion contest
ioncore = { 
  load_module = function() 
    return 1
  end
}

function dopath() 
end

_G["mod_xinerama"] = {
  query_screens = function() end
}

-- load xinerama module lua code
dofile('mod_xinerama.lua')

-- now perform some tests:
local screens = {{x=0,y=0,w=800,h=600}}
local result = mod_xinerama.merge_contained_screens(screens)
assert(table.getn(result) == 1)

result = mod_xinerama.merge_overlapping_screens(screens)
print(table.getn(result))
assert(table.getn(result) == 1)

screens = {{x=0,y=0,w=800,h=600}, {x=400,y=400,w=800,h=600}, {x=500,y=0,w=500,h=300}}
result = mod_xinerama.merge_overlapping_screens(screens)
print(table.getn(result))
assert(table.getn(result) == 2)
