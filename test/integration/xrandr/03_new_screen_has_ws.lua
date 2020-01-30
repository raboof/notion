-- TEMP
-- See why it's not finding mod_xrandr_mock
-- overwrite some methods in mod_xrandr to mock it
print('Mocking RandR inplementation')

local outputOne = { name = 'ONE', x = 0, y = 0, w = 1280, h = 960 }
local outputTwo = { name = 'TWO', x = 1280, y = 0, w = 1280, h = 1024 }

local n_screens = 2

function mod_xrandr.get_outputs(reg)
    if reg:geom().x == 0 then
        return { ONE = outputOne }
    elseif n_screens > 1 then
        return { TWO = outputTwo }
    else
        return {}
    end
end

function mod_xrandr.get_all_outputs()
    if n_screens > 1 then
        return { ONE = outputOne, TWO = outputTwo }
    else
        return { ONE = outputOne }
    end
end

function mx_names(mx)
    local result = ''
    mx:mx_i(function(child) result = result .. child:name() .. ', ' return true end)
    return result
end

mod_xrandr_mock = {}
function mod_xrandr_mock.set_number_of_screens(n)
    n_screens = n
    if n == 1 then
        os.execute("cp xrandr/fakexinerama-1monitor xrandr/.fakexinerama")
    else
        os.execute("cp xrandr/fakexinerama-2monitors xrandr/.fakexinerama")
    end
end
-- /TEMP

mod_xrandr.screenlayoutupdated()

if (notioncore.find_screen_id(1) == Nil) then
  return "Number of screens should be 2, again at the start of this test"
end

if (notioncore.find_screen_id(0):mx_count() < 1) then
  return "Screen 0 had no workspaces!"
elseif (notioncore.find_screen_id(1):mx_count() < 1) then
  return "Screen 1 had no workspaces!"
end

if notioncore.find_screen_id(0):mx_nth(0):name() ~= 'WGroupWS' then
  return "First workspace not correctly returned to first screen"
end
if notioncore.find_screen_id(1):mx_nth(0):name() ~= 'WGroupWS<1>' then
  return "Second workspace not correctly returned to second screen"
end

return "ok"
