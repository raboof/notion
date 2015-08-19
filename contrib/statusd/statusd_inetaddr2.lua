-- Authors: Guillermo Bonvehí <gbonvehi@gmail.com>
-- License: Public domain
--
-- statusd_inetaddr2.lua -- show IP address for Notion's statusbar

-- Shows assigned IP address for each configured interface.

-- Uses /sbin/ifconfig to fetch current iface's data

-- Configuration:

-- `template' string controls how the output will be formated.  It may
-- contain the following keywords:

-- PLACEHOLDER  DESCRIPTION             EXAMPLE
-- -----------  -----------             -------
-- iface        interface               eth0
-- ipv4         IPv4 address            192.168.1.1
-- ipv6         IPv6 address              ::1

-- `interfaces' table contains the interfaces to be monitored.
-- `separator' is placed between the entries in the output.
-- `interval' is the refresh timer (in milliseconds).

-- This software is in the public domain.

--------------------------------------------------------------------------------

local defaults = {
    template = "[%iface %ipv4 %ipv6]",
    interfaces = { "eth0", "wlan0" },
    separator = "  ",
    interval = 10*1000,
}
                            
local settings = table.join(statusd.get_config("inetaddr2"), defaults)
local inetaddr2_timer = statusd.create_timer()

local function get_inetaddr_ifcfg(iface)
    local r_table = {}
    local inet4_match = "inet addr:"
    local inet6_match = "inet6 addr:"
    local inet4_addr = "none"
    local inet6_addr = "none"
	local f = io.popen("/sbin/ifconfig " .. iface)
	if not f then
		return nil
    end
    while true do
        local line = f:read("*l")
        if line == nil then break end
        if (string.match(line, inet4_match)) then
            inet4_addr = line:match("inet addr:(%d+%.%d+%.%d+%.%d+)")
        elseif (string.match(line, inet4_match)) then
            inet6_addr = line:match(("([a-fA-F0-9]*):"):rep(8):gsub(":$","$"))
        end
    end
    f:close()
    r_table["iface"] = iface
    r_table["ipv4"] = inet4_addr
    r_table["ipv6"] = inet6_addr
    return r_table
end

local function update_inetaddr()
    local ifaces_info = {}
    for i=1, #settings.interfaces do
        local iface = settings.interfaces[i]
        local parsed_ifcfg = get_inetaddr_ifcfg(iface)
        if (parsed_ifcfg == nil) then
        else
            local s = string.gsub(settings.template, "%%(%w+)",
            function (arg)
                if (parsed_ifcfg[arg] ~= nil) then
                    return parsed_ifcfg[arg]
                end
                return nil
            end)
            ifaces_info[i] = s
        end
    end
    statusd.inform("inetaddr2", table.concat(ifaces_info, settings.separator))
    inetaddr2_timer:set(settings.interval, update_inetaddr)
end

update_inetaddr()
