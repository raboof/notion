-- Authors: Sadrul Habib Chowdhury <imadil@gmail.com>
-- License: Unknown
-- Last Changed: Unknown
-- 
-- USAGE: back-up the cfg_statusbar.lua. Rename this one to cfg_statusbar.lua,
-- make changes in settings.active and settings.inactive (look below) to meet
-- your needs (for farther changes, look into save_statusbar.lua, which will
-- be created by this script). Also notice init_settings and configurations.
-- Now restart ion3, right-click on the statusbar (or press F11) and see the
-- magic :)
--
-- Author: Sadrul Habib Chowdhury (imadil at gmail dot com)

ctrl_statusbar = {}

-- initializing settings for statusbar, as in mod_statusbar.create in cfg_statusbar.lua
if not ctrl_statusbar.init_settings then
  ctrl_statusbar.init_settings = {
    screen = 0,
    pos = 'bl',
    date_format='%a %Y-%b-%d %H:%M',
  }
end

-- settings for individual modules, as in mod_statusbar.launch_statusd in cfg_statusbar.lua
if not ctrl_statusbar.configurations then
  ctrl_statusbar.configurations = {
    -- Load meter
    --[[
    load={
        update_interval=10*1000,
        important_threshold=1.5,
        critical_threshold=4.0
    },
    --]]

    -- Mail meter
    --[[
    mail={
        update_interval=60*1000,
        mbox=os.getenv("MAIL")
    },
    --]]

    netmon={
        --device = "lo",
        show_avg=1,
        show_count = 0,
        --avg_sec = 60,
        --interval = 1*1000
    }
  }
end

if not ctrl_statusbar.settings then
  ctrl_statusbar.settings = {
    seperator = '||',
    opening = "[ ",
    closing = " ]",

    -- whatever you add in active will be put in a single string
    -- one after the other in the same order, and that's what you'll
    -- see in the statusbar. Also, take a look into save_statusbar.lua
    active = {
        "%date",
        "eth0: % %>netmon",
        "%schedule",
    },

    -- if there are monitors which you may want to use only occasionally,
    -- then put them in here
    inactive = {
        "uptime: %uptime",
        "%rss",
    },
    screen = 0,
    save_filename = "save_statusbar.lua", -- where are the settings saved?
    auto_save = 1,  -- should the changes be saved automatically?
  }
end

-- Construct the template for the statusbar
function ctrl_statusbar.get_template()
    local st = ctrl_statusbar.settings.opening
    for _, s in pairs(ctrl_statusbar.settings.active) do
        if _ > 1 then st = st .. " " .. ctrl_statusbar.settings.seperator .. " " .. s
        else st = st .. s
        end
    end
    st = st .. ctrl_statusbar.settings.closing
    return st
end

-- Refresh the statusbar
local function refresh_statusd()
    local st = ctrl_statusbar.get_template()
    local tbl = mod_statusbar.template_to_table(st)

    ctrl_statusbar.sb:set_template_table(tbl)

    if ctrl_statusbar.settings.auto_save == 1 then
        ctrl_statusbar.save_template()
    end
end

-- Add a new module
function ctrl_statusbar.add_module(mp, cmd)
    table.insert(ctrl_statusbar.settings.active, cmd)
    refresh_statusd()
end

-- Delete a module completely (Possible only for disabled modules)
function ctrl_statusbar.delete_module(ind)
    table.remove(ctrl_statusbar.settings.inactive, ind)
end

-- Change positions of two monitors
function ctrl_statusbar.exchange(from, to)
    table.insert(ctrl_statusbar.settings.active, to,
        table.remove(ctrl_statusbar.settings.active, from))
    refresh_statusd()
end

-- Move a monitor from active to inactive
function ctrl_statusbar.disable_active(_)
    table.insert(ctrl_statusbar.settings.inactive, 
            table.remove(ctrl_statusbar.settings.active, _))
    refresh_statusd()
end

-- Move a monitor from inactive to active
function ctrl_statusbar.insert_active(_, ind)
    if ind == -1 then
        table.insert(ctrl_statusbar.settings.active, 
                table.remove(ctrl_statusbar.settings.inactive, _))
    else
        table.insert(ctrl_statusbar.settings.active, ind,
                table.remove(ctrl_statusbar.settings.inactive, _))
    end
    refresh_statusd()
end

-- Save the templates
function ctrl_statusbar.save_template()
    local t = ioncore.get_paths()
    local f = io.open(t.userdir .. "/" .. ctrl_statusbar.settings.save_filename, "w")
    if not f then error(TR("couldn't save.")) return end

    local sv = "ctrl_statusbar.save = {\nactive = {\n"
    for _, s in pairs(ctrl_statusbar.settings.active) do
        sv = sv .. '"' .. s .. '",\n'
    end
    sv = sv .. "},\ninactive = {\n"
    for _, s in pairs(ctrl_statusbar.settings.inactive) do
        sv = sv .. '"' .. s .. '",\n'
    end
    sv = sv .. "}\n}"
    f:write(sv)
    f:close()
end

-- Create and initialize the statusbar
function ctrl_statusbar.init()
    dopath(ctrl_statusbar.settings.save_filename, true)    -- if anything is saved, read them
    if ctrl_statusbar.save then
        ctrl_statusbar.settings.active = ctrl_statusbar.save.active
        ctrl_statusbar.settings.inactive = ctrl_statusbar.save.inactive
    end
    ctrl_statusbar.init_settings.template = ctrl_statusbar.get_template()
    -- this is a very ugly hack to make sure all the statusd_ scripts get loaded
    for _, v in pairs(ctrl_statusbar.settings.inactive) do
        ctrl_statusbar.init_settings.template = ctrl_statusbar.init_settings.template .. " " .. v
    end
    ctrl_statusbar.sb = mod_statusbar.create(ctrl_statusbar.init_settings)
    mod_statusbar.launch_statusd(ctrl_statusbar.configurations)
    refresh_statusd()
end

-- Get just the name of a monitor
local function get_monitor_name(s)
    s = string.gsub(s, '%% ', '')   -- remove the "% " from the template
    s = string.gsub(s, '^.-%%[<>|]?0*[0-9]*(%w+).-$', '%1')
    return s
end

-- pardon the obfuscation, i don't know why i did that
function ctrl_statusbar.show_list(mplex)
    ret = {}
    for _, s in pairs(ctrl_statusbar.settings.active) do
        s = get_monitor_name(s)
        sub = {}

        for __, t in pairs(ctrl_statusbar.settings.active) do
            if _ ~= __ then
                t = get_monitor_name(t)
                if _ < __ then t = "Move after " .. t
                else t = "Move before " .. t
                end
                table.insert(sub, menuentry(t,
                        "ctrl_statusbar.exchange(\"" .. _ .. "\", \"" .. __ .. "\")"))
            end
        end
        
        table.insert(sub, menuentry("Disable " ..s,
                "ctrl_statusbar.disable_active(\"" .. _ .. "\")"))
        table.insert(ret, submenu(s, sub))
    end

    for _, s in pairs(ctrl_statusbar.settings.inactive) do
        s = get_monitor_name(s)
        sub = {}
        table.insert(sub, menuentry("Insert " ..s.." at the beginning",
                    "ctrl_statusbar.insert_active(\"" .. _ .. "\", \"1\")"))

        for __, t in pairs(ctrl_statusbar.settings.active) do
            t = get_monitor_name(t)
            table.insert(sub, menuentry("Insert after " .. t,
                    "ctrl_statusbar.insert_active(\"" .. _ .. "\", \"" .. (__+1) .. "\")"))
        end
        
        table.insert(sub, menuentry("Delete " ..s,
                "ctrl_statusbar.delete_module(\"" .. _ .. "\")"))
        table.insert(ret, submenu('! ' .. s, sub))
    end
    
    table.insert(ret, menuentry("Add a new Template",
        "mod_query.query(_, TR('New template'), nil, ctrl_statusbar.add_module, function () end, 'ctrl_statusbar')"))
    table.insert(ret, menuentry("Save template", ctrl_statusbar.save_template))
    
    return ret
end

ctrl_statusbar.init()

ioncore.defbindings("WScreen", {
    kpress(MOD1.."V", "mod_menu.bigmenu(_, _sub, ctrl_statusbar.show_list)")
})

ioncore.defbindings("WStatusBar", {
    mpress("Button3", "mod_menu.pmenu(_, _sub, ctrl_statusbar.show_list)"),
})

