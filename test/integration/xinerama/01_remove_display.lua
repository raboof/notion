-- Removing screens happens asynchronously, so we just check we start with 2 screens,
-- remove one, and then leave checking that one was removed up to the next test.

os.execute("cp xinerama/fakexinerama-2monitors xinerama/.fakexinerama");

mod_xinerama.refresh();

if (notioncore.find_screen_id(1) == Nil) then
  return "Initial number of screens should be 2, second screen not found"
elseif (notioncore.find_screen_id(2) ~= Nil) then
  return "Initial number of screens should be 2, third screen found"
end

-- Remove one display:
os.execute("cp xinerama/fakexinerama-1monitor xinerama/.fakexinerama")

mod_xinerama.refresh()

return "ok"
