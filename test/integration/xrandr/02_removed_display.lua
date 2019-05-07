mod_xrandr.screenlayoutupdated()

if notioncore.find_screen_id(1) then
  return "New number of screens should be 1, found ", notioncore.find_screen_id(1):name()
end

if notioncore.find_screen_id(0):mx_count() ~= 2 then
  return "Remaining screen should have 2 workspaces instead of " .. notioncore.find_screen_id(0):mx_count() ..
    ': ' .. mx_names(notioncore.find_screen_id(0))
end

print('Restoring second display and updating layout')
mod_xrandr_mock.set_number_of_screens(2)
mod_xrandr.screenlayoutupdated()

return "ok"
