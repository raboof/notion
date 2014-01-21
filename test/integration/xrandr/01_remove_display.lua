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
