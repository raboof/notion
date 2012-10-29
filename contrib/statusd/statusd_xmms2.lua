-- Authors: Voker57 <voker57@gmail.com>
-- License: Public domain
-- Last Changed: Unknown
--
-- Real simple xmms2 monitor

-- %xmms2 - current song in format "Artist - Title"

-- by Voker57 <voker57@gmail.com>
-- Public domain

local defaults={
    -- 500 or less makes seconds increment relatively smoothly while playing
    update_interval=500
}

local settings=table.join(statusd.get_config("xmms2"), defaults)

local xmms2_timer

local function get_xmms2_status()
	local xmms2 = io.popen("xmms2 current")
	if xmms2 == nil then
		return nil
	else
		local r = ""
		local t = ""
		repeat
			r = r..t
			t = xmms2:read()
		until t == nil
		return r
	end
end

local function update_xmms2()
	local xmms2_st = get_xmms2_status()
	statusd.inform("xmms2", xmms2_st)
	xmms2_timer:set(settings.update_interval, update_xmms2)
end

-- Init
xmms2_timer=statusd.create_timer()
update_xmms2()
