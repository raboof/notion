-- For honest workspaces, the initial outputs information, which determines the 
-- physical screen that a workspace wants to be on, is part of the C class 
-- WGroupWS. For "full screen workspaces" and scratchpads, we only keep this 
-- information in a temporary list.
InitialOutputs={}

function getInitialOutputs(ws)
    if obj_is(ws, "WGroupCW") or is_scratchpad(ws) then
        return InitialOutputs[ws:name()]
    elseif obj_is(ws, "WGroupWS") then
        return WGroupWS.get_initial_outputs(ws)
    else
        return nil
    end
end

function setInitialOutputs(ws, outputs)
    if obj_is(ws, "WGroupCW") or is_scratchpad(ws) then
        InitialOutputs[ws:name()] = outputs
    elseif obj_is(ws, "WGroupWS") then
        WGroupWS.set_initial_outputs(ws, outputs)
    end
end

function nilOrEmpty(t)
    return not t or empty(t) 
end

function mod_xrandr.workspace_added(ws)
    if nilOrEmpty(getInitialOutputs(ws)) then
        outputs = mod_xrandr.get_outputs(ws:screen_of(ws))
        outputKeys = {}
        for k,v in pairs(outputs) do
            table.insert(outputKeys, k)
        end
        setInitialOutputs(ws, outputKeys)
    end
    return true
end

function for_all_workspaces_do(fn)
    local workspaces={}
    notioncore.region_i(function(scr)
        scr:managed_i(function(ws)
            table.insert(workspaces, ws)
            return true 
        end)
        return true
    end, "WScreen")
    for _,ws in ipairs(workspaces) do
        fn(ws)
    end
end

function mod_xrandr.workspaces_added()
    for_all_workspaces_do(mod_xrandr.workspace_added)
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
    return package.loaded["mod_sp"] and mod_sp.is_scratchpad(ws)
end

function find_scratchpad(screen)
    local sp
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

    if workspace:screen_of() ~= screen then
        if is_scratchpad(workspace) then
        -- Moving a scratchpad to another screen is not meaningful, so instead we move 
        -- its content
            local content={}
            workspace:bottom():managed_i(function(reg)
                table.insert(content, reg)
                return true
            end)
            local sp=find_scratchpad(screen)
            for _,reg in ipairs(content) do
                sp:bottom():attach(reg)
            end
            return
        end

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

    local all_outputs = mod_xrandr.get_all_outputs()

    -- When moving a "full screen workspace" to another screen, we seem to lose 
    -- its placeholder and thereby the possibility to return it from full 
    -- screen later. Let's therefore try to close any full screen workspace 
    -- before rearranging.
    full_screen_workspaces={}
    for_all_workspaces_do(function(ws)
        if obj_is(ws, "WGroupCW") then table.insert(full_screen_workspaces, ws) 
        end
        return true
    end)
    for _,ws in ipairs(full_screen_workspaces) do
        ws:set_fullscreen("false")
    end

    -- round one: divide workspaces in directly assignable,
    -- orphans and wanderers
    function roundone(workspace)
        local screens = candidate_screens_for_outputs(all_outputs, getInitialOutputs(workspace))
        if not screens or empty(screens) then
            table.insert(orphans, workspace)
        elseif singleton(screens) then
            add_safe(new_mapping, firstValue(screens):id(), workspace)
        else
            wanderers[workspace] = screens
        end
        return true
    end
    for_all_workspaces_do(roundone)

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
