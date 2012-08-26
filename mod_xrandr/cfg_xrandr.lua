-- map workspace name to list of initial outputs for that workspace
initialScreens = {}

function nilOrEmpty(t)
    return not t or empty(t) 
end

function mod_xrandr.workspace_added(ws)
    if nilOrEmpty(initialScreens[ws:name()]) then
        outputs = mod_xrandr.get_outputs(ws:screen_of(ws))
        outputKeys = {}
        for k,v in pairs(outputs) do
          table.insert(outputKeys, k)
        end
        initialScreens[ws:name()] = outputKeys
    end
    return true
end

function mod_xrandr.workspaces_added()
    notioncore.region_i(mod_xrandr.workspace_added, "WGroupWS")
end

function mod_xrandr.screenmanagedchanged(tab) 
    if tab.mode == 'add' then
        mod_xrandr.workspace_added(tab.sub);
    end
end

screen_managed_changed_hook = notioncore.get_hook('screen_managed_changed_hook')
if screen_managed_changed_hook then
    screen_managed_changed_hook:add(mod_xrandr.screenmanagedchanged)
end

post_layout_setup_hook = notioncore.get_hook('ioncore_post_layout_setup_hook')
post_layout_setup_hook:add(mod_xrandr.workspaces_added)

function add_safe(t, key, value)
    if t[key] == nil then
        t[key] = {}
    end

    table.insert(t[key], value)
end

-- parameter: list of output names
-- returns: map from screen name to screen
function candidate_screens_for_output(all_outputs, outputname)
    local retval = {}

    function addIfContainsOutput(screen)
        local outputs_within_screen = mod_xrandr.get_outputs_within(all_outputs, screen)
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
function candidate_screens_for_outputs(all_outputs, outputnames)
    local result = {}

    if outputnames == nil then return result end

    for i,outputname in pairs(outputnames) do
        local screens = candidate_screens_for_output(all_outputs, outputname)
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
    return not next(t)
end

function singleton(t)
    local first = next(t)
    return first and not next(t, first) 
end

function is_scratchpad(ws)
    return ws:name():find('scratchws')
end

function find_scratchpad()
    local sp
    local screen = notioncore.find_screen_id(0)
    screen:managed_i(function(ws)
        if is_scratchpad(ws) then
            sp=ws
            return false
        else
            return true
        end
    end)
    return sp
end

function move_if_needed(workspace, screen_id)
    local screen = notioncore.find_screen_id(screen_id) 

    -- scratchpads are not part of a screens' mplex
    if workspace:screen_of() ~= screen then
        if not is_scratchpad(workspace) then
            screen:attach(workspace)
        else
            local sp=find_scratchpad()
            local cwins={}
            workspace:bottom():managed_i(function(cwin)
                table.insert(cwins, cwin)
                return false
            end)
            for k,cwin in ipairs(cwins) do
                sp:bottom():attach(cwin)
            end
        end
    end
end

function mod_xrandr.rearrangeworkspaces()
    -- for each screen id, which workspaces should be on that screen
    new_mapping = {}
    -- workspaces that want to be on an output that's currently not on any screen
    orphans = {}
    -- workspaces that want to be on multiple available outputs
    wanderers = {}

    local all_outputs = mod_xrandr.get_all_outputs();

    -- round one: divide workspaces in directly assignable,
    -- orphans and wanderers
    function roundone(workspace)
        local screens = candidate_screens_for_outputs(all_outputs, initialScreens[workspace:name()])
        if not screens or empty(screens) then
            table.insert(orphans, workspace)
        elseif singleton(screens) then
            add_safe(new_mapping, firstValue(screens):id(), workspace)
        else
            wanderers[workspace] = screens
        end
        return true
    end
    notioncore.region_i(roundone, "WGroupWS")

    for workspace,screens in pairs(wanderers) do
        -- TODO add to screen with least # of workspaces instead of just the 
        -- first one that applies
        if screens[workspace:screen_of():name()] then
            add_safe(new_mapping, workspace:screen_of():id(), workspace)
        else
            add_safe(new_mapping, firstValue(screens):id(), workspace)
        end
    end
    for i,workspace in pairs(orphans) do
        -- TODO add to screen with least # of workspaces instead of just the first one
        add_safe(new_mapping, 0, workspace)
    end
    for screen_id,workspaces in pairs(new_mapping) do
        -- move workspace to that 
        for i,workspace in pairs(workspaces) do
            move_if_needed(workspace, screen_id)
        end
    end
    mod_xinerama.populate_empty_screens()
end

-- refresh xinerama and rearrange workspaces on screen layout updates
function mod_xrandr.screenlayoutupdated()
    notioncore.profiling_start('notion_xrandrrefresh.prof')
    mod_xinerama.refresh()
    mod_xrandr.rearrangeworkspaces()
    notioncore.profiling_stop()
end

randr_screen_change_notify_hook = notioncore.get_hook('randr_screen_change_notify')

if randr_screen_change_notify_hook then
    randr_screen_change_notify_hook:add(mod_xrandr.screenlayoutupdated)
end
