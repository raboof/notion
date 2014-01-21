--
-- ion/mod_statusbar/ion-statusd/statusd_load.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2009.
--
-- See the included file LICENSE for details.
--

--
-- We should really use getloadavg(3) instead and move the meter to
-- Ion side to get properly up-to-date loads. But until such an export
-- is made, and we use potentially blocking files and external programs, 
-- this meter must be in ion-statusd.
--

local defaults={
    update_interval=10*1000,
    load_hint="1min",
    important_threshold=1.5,
    critical_threshold=4.0
}

local settings=table.join(statusd.get_config("load"), defaults)

local load_timer

local function get_hint(v)
    local i="normal"
    if v then
        if v>settings.critical_threshold then
            i="critical"
        elseif v>settings.important_threshold then
            i="important"
        end
    end
    return i
end

local function fmt(l)
    if not l then
        return "?"
    else
        return string.format("%0.2f", l)
    end
end

local function update_load()
    local lds = statusd.getloadavg()
    f1, f5, f15 = fmt(lds["1min"]), fmt(lds["5min"]), fmt(lds["15min"])
    statusd.inform("load", f1..", "..f5..", "..f15)
    statusd.inform("load_hint", get_hint(lds[settings.load_hint]))
    statusd.inform("load_1min", f1)
    statusd.inform("load_1min_hint", get_hint(lds["1min"]))
    statusd.inform("load_5min", f5)
    statusd.inform("load_5min_hint", get_hint(lds["5min"]))
    statusd.inform("load_15min", f15)
    statusd.inform("load_15min_hint", get_hint(lds["15min"]))
    load_timer:set(settings.update_interval, update_load)
end

-- Init
--statusd.inform("load_template", "0.00, 0.00, 0.00");

load_timer=statusd.create_timer()
update_load()


