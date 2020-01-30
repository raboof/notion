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

if not notioncore.find_screen_id(0) then
  return "Initial number of screens should be 2, first screen not found"
elseif not notioncore.find_screen_id(1) then
  return "Initial number of screens should be 2, second screen not found"
elseif notioncore.find_screen_id(2) then
  return "Initial number of screens should be 2, third screen found"
end

if notioncore.find_screen_id(0):mx_count() ~= 1 then
  return "Initial screen 0 should have 1 workspace instead of " .. notioncore.find_screen_id(0):mx_count()
end
print ('Screen 0: ' .. notioncore.find_screen_id(0):mx_nth(0):name() )
if notioncore.find_screen_id(1):mx_count() ~= 1 then
  return "Initial screen 1 should have 1 workspace instead of " .. notioncore.find_screen_id(1):mx_count()
end
print ('Screen 1: ' .. notioncore.find_screen_id(1):mx_nth(0):name())

os.execute("cp xrandr/fakexinerama-2monitors xrandr/.fakexinerama")

print('Updating layout when there is 2 screens present')
-- Removing screens happens asynchronously, so we just check we start with 2 screens,
-- remove one, and then leave checking that one was removed up to the next test.
mod_xrandr.screenlayoutupdated()

if notioncore.find_screen_id(0):mx_count() ~= 1 then
  return "After updating screen 0 should have 1 workspaces instead of " ..
    notioncore.find_screen_id(0):mx_count() .. ": " .. mx_names(notioncore.find_screen_id(0))
end
if notioncore.find_screen_id(1):mx_count() ~= 1 then
  return "After updating screen 1 should have 1 workspace instead of " .. notioncore.find_screen_id(1):mx_count()
end

print('Removing one screen and updating layout')
mod_xrandr_mock.set_number_of_screens(1)
mod_xrandr.screenlayoutupdated()

return "ok"
