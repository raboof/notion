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

-- {{{ mock notion context
_G["statusd"] = {
  get_config = function(config_section)
    if config_section == 'inetaddr2' then
      return {
        interfaces = { "enp4s5" },
      }
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

-- load ioncore_luaext module lua code
dofile('../../../ioncore/ioncore_luaext.lua')

-- load inetaddr2
dofile('statusd_inetaddr2.lua')

function do_test(fn_name, input, output)
    print("\nTesting: " .. fn_name)
	ret = statusd_inetaddr2_export.get_inetaddr_ifcfg("enp4s5")
    print("\nInput: ")
    print(input)
    print_tables("Output", ret)
    -- print_tables("Expected Output", output)
    assert(table_compare(output, ret))
	assert(false, "TODO: get_inetaddr_ifcfg only parses ip addr show output, output parameter is ignored, more refactor is needed.")
end

-- now perform some tests:

-- {{{ test given input

do_test("get_ip_address_using__ip_addr",
[[
2: enp4s5: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 1000
    link/ether 00:08:54:68:7e:2c brd ff:ff:ff:ff:ff:ff
    inet 192.168.1.122/24 brd 192.168.1.255 scope global enp4s5
       valid_lft forever preferred_lft forever
    inet6 fe80::4369:c26a:b94c:1b49/64 scope link 
       valid_lft forever preferred_lft forever
]],
{
  iface = "enp4s5", ipv4 = "192.168.1.122", ipv6 = "fe80::4369:c26a:b94c:1b49"
})

do_test("get_ip_address_using__ifconfig",
[[
enp4s5: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
        inet 192.168.1.122  netmask 255.255.255.0  broadcast 192.168.1.255
        inet6 fe80::4369:c26a:b94c:1b49  prefixlen 64  scopeid 0x20<link>
        ether 00:08:54:68:7e:2c  txqueuelen 1000  (Ethernet)
        RX packets 25871  bytes 13467780 (12.8 MiB)
        RX errors 0  dropped 4  overruns 0  frame 0
        TX packets 18657  bytes 3483623 (3.3 MiB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
]],
{
  iface = "enp4s5", ipv4 = "192.168.1.122", ipv6 = "fe80::4369:c26a:b94c:1b49"
})

-- }}}
