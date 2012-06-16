mod_xinerama.refresh();

if (notioncore.find_screen_id(1) ~= Nil) then
  return "New number of screens should be 1, found " .. (notioncore.find_screen_id(1):name())
end

os.execute("cp xinerama/fakexinerama-2monitors xinerama/.fakexinerama");

return "ok"
