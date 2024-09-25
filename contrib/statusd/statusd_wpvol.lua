--[[ Authors: Pavel Karnaukhov <chertyhansky@yandex.ru>
License: GPLv3
Shows volume using wpctl - WirePlumber, PipeWire session manager

Add to template section of your cfg_statusbar.lua:
%wpvol_volume -- shows volume of @DEFAULT_SINK@ in %
%wpvol_state -- shows 'M' if muted and ' ' if unmuted

You can edit device name and update interval in local defaults
]]


local defaults = {
	update_interval = 1*1000,
	important = 33,
	critical = 66,
	device = '@DEFAULT_SINK@',
	unknown = '?'
}

local settings = table.join(statusd.get_config('wpvol'), defaults)

local function get_volume()
	local f=io.popen('wpctl get-volume '..settings.device)
	local s=f:read('*all')
	f:close()
	local _, _, volume, state = string.find(s, 'Volume: ([0-9].[0-9]*)(.*)')

	if not volume then
		return settings.unknown, settings.unknown, critical, critical
	else
		volume = math.floor(volume * 100)
	   if volume >= 0 and volume < settings.important then
		   volume_hint = 'normal'
	   elseif volume >= settings.important and volume < settings.critical then
		   volume_hint = 'important'
	   elseif volume >= settings.critical then
		   volume_hint = 'critical'
	   end
	end

	if state == ' [MUTED]\n' then
		state = 'M'
		state_hint = 'critical'
	else
		state = '.'
		state_hint = 'normal'
	end

	return volume, state, volume_hint, state_hint
end


local function inform(key, value)
   statusd.inform('wpvol_'..key, value)
end


local volume_timer = statusd.create_timer()

local function update_volume()
   local volume, state, volume_hint, state_hint = get_volume()
   inform('volume', volume..'%')
   inform('state', state)
   inform('volume_hint', volume_hint)
   inform('state_hint', state_hint)

   -- update interval
   volume_timer:set(settings.update_interval, update_volume)
end

update_volume()
