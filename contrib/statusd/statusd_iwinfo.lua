-- statusd_iwinfo.lua
--
-- Copyright (c) Relu Patrascu 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--
-- Nov. 5, 2004
-- Disclaimer
-- Neither am I a lua expert nor do I have the time to invest in writing
-- better code here.  I simply needed this utility and it works OK for me.
-- I give no guarantees of any kind for this code.  Suggestions for
-- improvement are welcome.
-- ikoflexer at gmail dot com

-- Nov 17, 2004
-- ikoflexer at gmail dot com
--
-- Made it return a space " " instead of empty string ""
-- when /proc/net/wireless doesn't report any interface.
-- Otherwise old info lingers in the status bar.

if not statusd_iwinfo then
  statusd_iwinfo = { interval = 10*1000 }
end

local iwinfo_timer

local function get_iwinfo_iwcfg()
	local f = io.open('/proc/net/wireless', 'r')
	if not f then
		return
	end
-- first 2 lines -- headers
	f:read('*l')
	f:read('*l')
-- the third line has wifi info
	local s = f:read('*l')
	f:close()
	local st, en, iface = 0, 0, 0
	if not s then
		return
	end
	st, en, iface = string.find(s, '(%w+):')
	local f1 = io.popen("/sbin/iwconfig " .. iface)
	if not f1 then
		return
	else
		local iwOut = f1:read('*a')
		f1:close()
		st,en,proto = string.find(iwOut, '(802.11[%-]*%a*)')
		st,en,ssid = string.find(iwOut, 'ESSID[=:]"([%w+%s*]*)"', en)
		st,en,bitrate = string.find(iwOut, 'Bit Rate[=:]([%s%w%.]*%/%a+)', en)
		bitrate = string.gsub(bitrate, "%s", "")
		st,en,linkq = string.find(iwOut, 'Link Quality[=:](%d+%/%d+)', en)
		st,en,signal = string.find(iwOut, 'Signal level[=:](%-%d+)', en)
		st,en,noise = string.find(iwOut, 'Noise level[=:](%-%d+)', en)

		return proto, ssid, bitrate, linkq, signal, noise
	end
end

local function update_iwinfo()
	local proto, ssid, bitrate, linkq, signal, noise = get_iwinfo_iwcfg()

-- In case get_iwinfo_iwcfg doesn't return any values we don't want stupid lua
-- errors about concatenating nil values.
	ssid = ssid or "N/A"
	bitrate = bitrate or "N/A"
	linkq = linkq or "N/A"
	signal = signal or "N/A"
	noise = noise or "N/A"
	proto = proto or "N/A"

	statusd.inform("iwinfo_cfg", ssid.." "..bitrate.." "..linkq.." "..signal.."/"..noise.."dBm "..proto)
	statusd.inform("iwinfo_ssid", ssid)
	statusd.inform("iwinfo_bitrate", bitrate)
	statusd.inform("iwinfo_linkq", linkq)
	statusd.inform("iwinfo_signal", signal)
	statusd.inform("iwinfo_noise", noise)
	statusd.inform("iwinfo_proto", proto)
	iwinfo_timer:set(statusd_iwinfo.interval, update_iwinfo)
end

-- Init
iwinfo_timer = statusd.create_timer()
update_iwinfo()
