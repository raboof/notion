-- Authors: Guillermo Bonvehí <gbonvehi@gmail.com>
-- License: Public domain
--
-- statusd_inetaddr2.lua -- show IP address for Notion's statusbar

-- Shows assigned IP address for each configured interface.

-- Uses `ip addr show` or `ifconfig` to fetch current iface's data

-- Configuration:

-- `template' string controls how the output will be formated.  It may
-- contain the following keywords:

-- PLACEHOLDER  DESCRIPTION             EXAMPLE
-- -----------  -----------             -------
-- iface        interface               eth0
-- ipv4         IPv4 address            192.168.1.1
-- ipv6         IPv6 address            fe80::208:54ff:fe68:7e2c

-- `interfaces' table contains the interfaces to be monitored.
-- `separator' is placed between the entries in the output.
-- `interval' is the refresh timer (in milliseconds).
-- `mode` use `ip` for `ip addr show` or `ifconfig` for `ifconfig`

-- This software is in the public domain.

--------------------------------------------------------------------------------

local defaults = {
    template = "[%iface %ipv4 %ipv6]",
    interfaces = { "eth0", "wlan0" },
    separator = "  ",
    interval = 10*1000,
    mode = "ip",
    mode_path_check = true,
}
                            
local settings = table.join(statusd.get_config("inetaddr2"), defaults)
local inetaddr2_timer = statusd.create_timer()
local mode_path = ""

local function file_exists(name)
    local f=io.open(name,"r")
    if f~=nil then io.close(f) return true else return false end
 end

local function get_tool_output(command)
    local f = io.popen(command)
    if not f then
        return nil
    end
    lines = f:read("a")
    f:close()
    return lines
end

local function get_ip_address_using__ifconfig(iface, lines)
    local r_table = {}
    local inet4_match = "inet "
    local inet6_match = "inet6 "
    local inet4_addr = "none"
    local inet6_addr = "none"
    lines = lines or get_tool_output("LC_ALL=C " .. mode_path .. " " .. iface .. " 2>/dev/null")
    for line in lines:gmatch("([^\n]*)\n?") do
        if (string.match(line, inet4_match)) then
            inet4_addr = line:match("inet (%d+%.%d+%.%d+%.%d+)") or "none"
        elseif (string.match(line, inet6_match)) then
            inet6_addr = line:match("inet6 ([a-fA-F0-9%:]+) ") or "none"
        end
    end
    r_table["iface"] = iface
    r_table["ipv4"] = inet4_addr
    r_table["ipv6"] = inet6_addr
    return r_table
end

local function get_ip_address_using__ip_addr(iface, lines)
    local r_table = {}
    local inet4_match = "inet "
    local inet6_match = "inet6 "
    local inet4_addr = "none"
    local inet6_addr = "none"
    lines = lines or get_tool_output("LC_ALL=C " .. mode_path .. " addr show " .. iface .. " 2>/dev/null")
    for line in lines:gmatch("([^\n]*)\n?") do
        if (string.match(line, inet4_match)) then
            inet4_addr = line:match("inet (%d+%.%d+%.%d+%.%d+)") or "none"
        elseif (string.match(line, inet6_match)) then
            inet6_addr = line:match("inet6 ([a-fA-F0-9%:]+)/") or "none"
        end
    end
    r_table["iface"] = iface
    r_table["ipv4"] = inet4_addr
    r_table["ipv6"] = inet6_addr
    return r_table
end

local function get_inetaddr_ifcfg(mode, iface)
    if mode == "ifconfig" then
        return get_ip_address_using__ifconfig(iface)
    elseif mode == "ip" then
        return get_ip_address_using__ip_addr(iface)
    end
end

local function update_inetaddr()
    local ifaces_info = {}
    for i=1, #settings.interfaces do
        local iface = settings.interfaces[i]
        local parsed_ifcfg = get_inetaddr_ifcfg(settings.mode, iface)
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

inetaddr2_timer:set(100, update_inetaddr)

if settings.mode == "ip" then
    ip_paths = {"/sbin/ip", "/usr/sbin/ip", "/bin/ip", "/usr/bin/ip"}
elseif settings.mode == "ifconfig" then
    ip_paths = {"/sbin/ifconfig", "/usr/sbin/ifconfig", "/bin/ifconfig", "/usr/bin/ifconfig"}
end
for i = 1, #ip_paths do
    if file_exists(ip_paths[i]) then
        mode_path = ip_paths[i]
        break
    end
end
if (settings.mode_path_check == true) then
    assert(mode_path ~= "", "Could not find a suitable binary for " .. settings.mode)
end

statusd_inetaddr2_export = {
    get_ip_address_using__ifconfig = get_ip_address_using__ifconfig,
    get_ip_address_using__ip_addr = get_ip_address_using__ip_addr,
}
