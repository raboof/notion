--[[
Author: Etan Reisner
Email: deryni@gmail.com
Summary: Adds a handful of statusbar meters displaying the current workspace name, all workspace names, etc. in a variety of manners.
Version: 0.6
Last Updated: 2007-01-23

Copyright (c) Etan Reisner 2007
--]]

-- Usage:
-- Add %wsname (or %wsname_full) to your statusbar template.
--
-- For multiple head setups with multiple statusbars,
-- use %wsname_{screennum} in your template (%wsname is the same as %wsname_0).
--
-- The %wsname_fullall meter shows a combined %wsname_full display for all screens.
--
-- The %wsname_pre meter contains the workspaces earlier in the list than the
-- current workspace.
--
-- The %wsname_post meter contains the workspaces later in the list than the
-- current workspace.

if not mod_statusbar then
    return
end

local defaults = {
    marker = "*",
    template = "xxxxxxxxxxx",
    all_marker = " - ",
}

local wsname = table.join(wsname or {}, defaults)

local function get_ws_name(t)
    local reg = t["reg"]

    if not obj_is(reg, "WScreen") then
        return
    end

    local ws_names_all, wsname_pre, wsname_post = nil, "", ""

    local function inform(screen)
        if screen:mx_count() == 0 then
            return
        end

        local ws_names, before, id = nil, true, screen:id()

        local function compose_ws_names(ws)
            local marker, current = "", false
            local wsn = ws:name() or "?"

            if ws == screen:mx_current() then
                marker = wsname.marker
                before = false
                current = true
            end

            if not ws_names then
                ws_names = marker..wsn..marker
            else
                ws_names = ws_names.." "..marker..wsn..marker
            end

            if before and not current then
                if wsname_pre == "" then
                    wsname_pre = wsn
                else
                    wsname_pre = wsname_pre.." "..wsn
                end
            elseif not current then
                if wsname_post == "" then
                    wsname_post = wsn
                else
                    wsname_post = wsname_post.." "..wsn
                end
            end

            return true
        end

        screen:mx_i(compose_ws_names)

        mod_statusbar.inform("wsname_"..id, screen:mx_current():name())
        mod_statusbar.inform("wsname_full_"..id, ws_names)
        mod_statusbar.inform("wsname_full_"..id.."_template", ws_names:len())
        mod_statusbar.inform("wsname_pre_"..id, wsname_pre)
        mod_statusbar.inform("wsname_pre_"..id.."_template", wsname_pre:len())
        mod_statusbar.inform("wsname_post_"..id, wsname_post)
        mod_statusbar.inform("wsname_post_"..id.."_template", wsname_post:len())

        if id == 0 then
            mod_statusbar.inform("wsname", screen:mx_current():name())
            mod_statusbar.inform("wsname_full", ws_names)
            mod_statusbar.inform("wsname_full_template", ws_names:len())
            mod_statusbar.inform("wsname_pre", wsname_pre)
            mod_statusbar.inform("wsname_pre_template", wsname_pre:len())
            mod_statusbar.inform("wsname_post", wsname_post)
            mod_statusbar.inform("wsname_post_template", wsname_post:len())
        end

        if not ws_names_all then
            ws_names_all = ws_names
        else
            ws_names_all = ws_names_all..wsname.all_marker..ws_names
        end

        return true
    end

    ioncore.region_i(inform, "WScreen")

    mod_statusbar.inform("wsname_fullall", ws_names_all)
    mod_statusbar.inform("wsname_fullall_template", string.len(ws_names_all))

    ioncore.defer(function() mod_statusbar.update() end)
end

local function setup_hooks()
    local hook

    hook = ioncore.get_hook("screen_managed_changed_hook")
    if hook then
        hook:add(get_ws_name)
    end
end

-- Init
setup_hooks()

local function inform(screen)
    local template
    local id = screen:id()

    if wsname[id] and wsname[id].template then
        template = wsname[id].template
    else
        template = wsname.template
    end

    mod_statusbar.inform("wsname_"..id.."_template", template)

    return true
end

mod_statusbar.inform("wsname_template", wsname.template)
ioncore.region_i(inform, "WScreen")

mod_statusbar.update(true)
