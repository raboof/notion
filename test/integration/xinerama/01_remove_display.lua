os.execute("cp fakexinerama-2monitor .fakexinerama");

mod_xinerama.refresh();

if (notioncore.find_screen_id(1) == Nil) then
  return "Initial number of screens should be 2"
end

-- Remove one display:
os.execute("cp fakexinerama-1monitor .fakexinerama");

mod_xinerama.refresh();

if (notioncore.find_screen_id(1) ~= Nil) then
  return "New number of screens should be 1"
end

return "ok"
