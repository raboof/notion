rofi = {}

rofi.active_menu_entries = {}

function rofi.menu_list(menu_name)
  local entry_lines = ""

  local entries = ioncore.getmenu(menu_name)()

  rofi.active_menu_entries[menu_name] = {}

  for i,entry in ipairs(entries) do
    entry_lines = entry_lines .. entry.name .. "\n"
    rofi.active_menu_entries[menu_name][entry.name] = entry.func
  end

  return entry_lines
end

function rofi.menu_action(menu_name, entry_name)
  rofi.active_menu_entries[menu_name][entry_name]()
end


-- To replace all menus with rofi set mod_menu.menu = rofi.menu
-- notion_rofi.sh must be moved to ~/.notion/ first
function rofi.menu(mplex, sub, menu, unused_parms)
    local helper = notioncore.get_paths()["userdir"].."/notion_rofi.sh"
    local rofispec = string.format("%s:%s %s", menu, helper, menu)

    ioncore.exec(string.format("rofi -show %s -modi '%s'", menu, rofispec))
end
