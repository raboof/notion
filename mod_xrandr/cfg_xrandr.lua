-- map workspace name to list of initial outputs for that workspace
initialScreens = {}

function screenmanagedchanged(tab) 
    if tab.mode == 'add' and initialScreens[tab.sub:name()] == nil then
        outputs = mod_xrandr.get_outputs(tab.reg)
        outputKeys = {}
        for k,v in pairs(outputs) do
          table.insert(outputKeys, k)
        end
        initialScreens[tab.sub:name()] = outputKeys
    end
end

screen_managed_changed_hook = notioncore.get_hook('screen_managed_changed_hook')
if screen_managed_changed_hook ~= nil then
    screen_managed_changed_hook:add(screenmanagedchanged)
end

function add_safe(t, key, value)
    if t[key] == nil then
        t[key] = {}
    end

    table.insert(t[key], value)
end

-- parameter: list of output names
-- returns: map from screen name to screen
function candidate_screens_for_output(outputname)
    local retval = {}

    function addIfContainsOutput(screen)
        local outputs_within_screen = mod_xrandr.get_outputs_within(screen)
        if outputs_within_screen[outputname] ~= nil then
            retval[screen:name()] = screen
        end
        return true
    end
    notioncore.region_i(addIfContainsOutput, "WScreen")

    return retval
end

-- parameter: list of output names
-- returns: map from screen name to screen
function candidate_screens_for_outputs(outputnames)
    local result = {}

    if outputnames == nil then return result end

    for i,outputname in pairs(outputnames) do
        local screens = candidate_screens_for_output(outputname)
        for k,screen in pairs(screens) do
             result[k] = screen;
        end
    end
    return result;
end

function firstValue(t)
   local key, value = next(t)
   return value
end

function firstKey(t)
   local key, value = next(t)
   return key
end

function empty(t)
    return next(t) == nil
end

function singleton(t)
    local first = next(t)
    return first ~= nil and next(t,first) == nil
end

function move_if_needed(workspace, screen_id)
    local screen = notioncore.find_screen_id(screen_id) 
    if workspace:screen_of() ~= screen then
        screen:attach(workspace)
    end
end

function mod_xrandr.rearrangeworkspaces()
    -- for each screen id, which workspaces should be on that screen
    new_mapping = {}
    -- workspaces that want to be on an output that's currently not on any screen
    orphans = {}
    -- workspaces that want to be on multiple available outputs
    wanderers = {}

    -- round one: divide workspaces in directly assignable,
    -- orphans and wanderers
    function roundone(workspace)
        local screens = candidate_screens_for_outputs(initialScreens[workspace:name()])
        if (screens == nil) or empty(screens) then
            table.insert(orphans, workspace)
        elseif singleton(screens) then
            local name, screen = next(screens)
            add_safe(new_mapping, screen:id(), workspace)
        else
            add_safe(wanderers, workspace, screens)
        end
        return true
    end
    notioncore.region_i(roundone, "WGroupWS")

    for workspace,screens in pairs(wanderers) do
        -- print('Wanderer', workspace:name())
        -- TODO add to screen with least # of workspaces instead of just the 
        -- first one that applies
        add_safe(new_mapping, screens[0]:id(), workspace)
    end
    for i,workspace in pairs(orphans) do
        -- print('Orphan', workspace:name())
        -- TODO add to screen with least # of workspaces instead of just the first one
        add_safe(new_mapping, 0, workspace)
    end
    for screen_id,workspaces in pairs(new_mapping) do
        -- move workspace to that 
        for i,workspace in pairs(workspaces) do
            move_if_needed(workspace, screen_id)
        end
    end
end

-- refresh xinerama and rearrange workspaces on screen layout updates
function screenlayoutupdated()
    mod_xinerama.refresh()
    mod_xrandr.rearrangeworkspaces()
end

randr_screen_change_notify_hook = notioncore.get_hook('randr_screen_change_notify')

if randr_screen_change_notify_hook ~= nil then
    randr_screen_change_notify_hook:add(screenlayoutupdated)
end
