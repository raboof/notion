-- test_statusd_inetaddr2.lua
-- 
-- Based on mod_xinerama/test_xinerama.lua
--
-- {{{ Table printing

function table_tostring (tt)
  if type(tt) == "table" then
    local sb = {}
    local first = true
    table.insert(sb, "{ ");
    for key, value in pairs (tt) do
      if first then first = false else table.insert(sb, ", ") end
      if "number" == type (key) then
	  table.insert(sb, table_tostring (value))
      else
	  table.insert(sb, key)
	  table.insert(sb, "=")
	  table.insert(sb, table_tostring (value))
      end
    end
    table.insert(sb, " }");
    return table.concat(sb)
  elseif type (tt) == "number" then
    return tostring(tt)
  else
    return '"' .. tostring(tt) .. '"'
  end
end

function print_tables(str,t)
    print(str .. ":")
    for key, value in pairs(t) do
      print("   " .. key .. ": " .. table_tostring(value))
    end
end

-- }}}

-- {{{ Table compare
-- Second table tree may contain some indices that the first does not contain.

function table_compare(table, super)
    if not ( type(table) == type(super) ) then
	return false
    end
    if not ( type(table) == "table" ) then
	return table == super
    end
    for key, value in pairs(table) do
	if not table_compare(value, super[key]) then return false end
    end
    return true
end
-- }}}

-- load ioncore_luaext module lua code
dofile('../../../ioncore/ioncore_luaext.lua')

-- {{{ mock notion context
_G["statusd"] = {
  get_config = function(config_section)
    if config_section == 'inetaddr2' then
      return { }
    end
  end,
  create_timer = function()
    return {
      set = function() end
    }
  end,
  inform = function() end
}
-- }}}

-- load inetaddr2
dofile('statusd_inetaddr2.lua')

function do_test(fn_name, input, output)
  print("\nTesting: " .. fn_name)
  print("\nInput: ")
  print(input["iface"])
  print(input["lines"])
  ret = statusd_inetaddr2_export[fn_name](input["iface"], input["lines"])
  print_tables("Output", ret)
  print_tables("Expected Output", output)
  assert(table_compare(output, ret))
end

-- now perform some tests:

-- {{{ test given input

do_test("get_ip_address_using__ip_addr",
{
  iface = "enp3s0f1",
  lines = [[
    2: enp3s0f1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UP group default qlen 1000
        link/ether 00:90:f5:f3:0f:bf brd ff:ff:ff:ff:ff:ff
        inet 10.4.61.144/24 brd 10.4.61.255 scope global dynamic noprefixroute enp3s0f1
           valid_lft 443398sec preferred_lft 443398sec
        inet6 fe80::e4f2:7b54:9f1c:c9e2/64 scope link noprefixroute
           valid_lft forever preferred_lft forever
    ]]
},
{
  iface = "enp3s0f1", ipv4 = "10.4.61.144", ipv6 = "fe80::e4f2:7b54:9f1c:c9e2"
})

do_test("get_ip_address_using__ip_addr",
{
  iface = "enp4s5",
  lines = 
[[
2: enp4s5: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 1000
    link/ether 00:08:54:68:7e:2c brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.122/24 brd 192.168.1.255 scope global enp4s5
       valid_lft forever preferred_lft forever
    inet6 fe80::4369:c26a:b94c:1b49/64 scope link 
       valid_lft forever preferred_lft forever
]]
},
{
  iface = "enp4s5", ipv4 = "192.168.1.122", ipv6 = "fe80::4369:c26a:b94c:1b49"
})

do_test("get_ip_address_using__ifconfig",
{
  iface = "enp4s5",
  lines = 
[[
enp4s5: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.1.122  netmask 255.255.255.0  broadcast 192.168.1.255
        inet6 fe80::4369:c26a:b94c:1b49  prefixlen 64  scopeid 0x20<link>
        ether 00:08:54:68:7e:2c  txqueuelen 1000  (Ethernet)
        RX packets 25871  bytes 13467780 (12.8 MiB)
        RX errors 0  dropped 4  overruns 0  frame 0
        TX packets 18657  bytes 3483623 (3.3 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
]]
},
{
  iface = "enp4s5", ipv4 = "192.168.1.122", ipv6 = "fe80::4369:c26a:b94c:1b49"
})

-- OpenIndiana
-- https://github.com/raboof/notion/pull/12#issuecomment-217928583
do_test("get_ip_address_using__ifconfig",
{
  iface = "dhcp0",
  lines = [[
lo0: flags=2001000849<UP,LOOPBACK,RUNNING,MULTICAST,IPv4,VIRTUAL> mtu 8232 index 1
    inet 127.0.0.1 netmask ff000000 
dhcp0: flags=1004843<UP,BROADCAST,RUNNING,MULTICAST,DHCP,IPv4> mtu 1500 index 2
    inet 10.10.1.34 netmask fffffe00 broadcast 10.10.1.255
  ]]
},
{
  iface = "dhcp0", ipv4 = "10.10.1.34", ipv6 = "none"
}
)

-- }}}
