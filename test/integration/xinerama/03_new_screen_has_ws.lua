
mod_xinerama.refresh()
mod_xinerama.populate_empty_screens()

if (notioncore.find_screen_id(1) == Nil) then
  return "Number of screens should be 2, again at the start of this test"
end

if (notioncore.find_screen_id(0):mx_count() < 1) then
  return "Screen 0 had no workspaces!"
elseif (notioncore.find_screen_id(1):mx_count() < 1) then
  return "Screen 1 had no workspaces!"
end

return "ok"
