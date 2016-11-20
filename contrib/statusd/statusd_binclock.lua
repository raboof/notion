-- Authors: Mario García H. <drosophila@nmental.com>
-- License: GPL, version 2
-- Last Changed: 2006-12-15
--
--------------------------------------------------------------------------------------
--
--	PURPOSE:
--	Binary clock for Ion3 in two posible formats:
--	
-->	Space waste: [0001:000111]  and   Alien Code: [ ..:" .  ..]
--	(You can modify the characters of 'alien' template to get a good looking clock)
--	
-->	This is an example template (could be better):  - -===^---  -  I will explain it.
--	
--	HOW TO READ:
--	In the "mono-character" Ion display, you need to read characters as : and =,
--	like two different characters: one at the top, one at the bottom.
--
--	In the example, equals are actually two different bars, one at the top and other
--	at the bottom. The little triangle (a square or some weird symbol if your font
--	doesn't support that character) represents a separator between hours and seconds so:
--	
--	- Upper-left represents hours: 2^4 .. 2^0 in 12 hours format.
--	  (4 characters from 'right to left' after separator)
--	- Bottom-left represents minutes: 2^6 ... 2^0
--	  (6 characters from 'right to left' after separator)
--	- Right side, after separator, represents seconds: 2^6 ... 2^0
--
--	Every character on dots code (AKA: alien) can be replaced:
--	- Bottom character or symbol ["_"]
--	- Upper character
--	- Both characters [Example: "=" or  ":" ]
--	- Separator character [Example: "^", "|"]
--
--	
------	USAGE:  ----------------------------------------------------------------------
--
--	Put [%binclock_10] on $HOME/.ion3/cfg_statusbar.lua to get a beauty
--	[0110:101100] or [%binclock] to get an ugly [. .:' ].
--	By my side, I preffer the ugly dots and numbers character display.
--	
--	It is relaxing for me to see the dots dancing so I added extra space to
--	show seconds. If you like that, don't forget to put this in settings:
--	binclock = { show_seconds = true }    *	See other options at the beggining
--	of the code.
--------------------------------------------------------------------------------------
--	
--	LICENSE:
--	GPL2 Copyright(C)2006 Mario García H.
--	(See http://www.gnu.org/licenses/gpl.html to read complete license)
--
--	T.STAMP: Fri Dec 15 13:37:54 2006
--
--	VERSION: 0.1.c
--
--	DEPENDS: No dependencies.
--
--	INSECTS:
--    * On non UTF-8 environments, default values for separator symbol and
--     	upper symbol are prone to not work. So, change them in settings please.
--   ** If you put the clock on the left corner the left black spaces will disapear,
--	use l_border (left border) or r_border characters in settings to avoid that.
--
--	LAST CHANGES:
--	0.1 -> 0.1.c
--	[ Changed 'while' for 'repeat' (It is almost 5X faster)]
--	[ Removed 2 useless tables ]
--	[ Changed if then else statements for logical statements:
--	  is more clean and goes a little faster]
--	[ Simplified some string concatenations (A real slow down on Lua code). ]
--	[ Some cleaning on this help text ]
--
--	CONTACT:
--	drosophila (at) nmental (dot) com
--
-----   SEE MORE DETAILED SETTINGS HERE: ---------------------------------------------

local defaults = {
	update_interval = 2000,
	show_seconds = true,			-- EXAMPLES:
	separator = "°" or string.char(176), 	-- "|", or "^",
	r_border = "",				-- "|", or ">"
	l_border = "",				-- "|", or "<"
	top_sym = "¯" or string.char(175), 	-- string.char(183), if your locals are not UTF-8
	low_sym = "-" or "_", 			-- ".",
	both_sym = "=", 			-- ":",
	empty_sym = [[ ]] or string.char(32),	--  string.char(168) --> 164, 168, 176, 179 ??
	color = "normal" 			-- "critical", "important", ...(white, red, green, etc.)
						-- It is not color, just and alias for alarm-types.
}
local settings = table.join(statusd.get_config("binclock"), defaults)
-------------------------------------------------------------------------------------

local binary_timer
local pattern = {}


local function convert_time()				--> Six cycles to get binary
	pattern = {}
	local hours, minutes, seconds =--
	tonumber(os.date("%I")), tonumber(os.date("%M")), tonumber(os.date("%S"))
	local i, j = 32, 1
	
	repeat
		pattern[j] = hours >=i and 1 or 0
		pattern[j + 7] = minutes >=i and 1 or 0
		pattern[j + 14] = seconds >=i and 1 or 0
		hours = hours >= i and hours - i or hours
		minutes = minutes >= i and minutes - i or minutes
		seconds = seconds >= i and seconds - i or seconds
		i, j = i/2, j + 1
	until j == 7
	
	pattern[7], pattern[14] = settings.separator, settings.separator
end


local function inform_dots()		  --> Six extra cycles to get alien chars aligned
	local dots, dots_s = "", ""
	--
	for i = 1, 6, 1 do  		
		dots = pattern[i] == 1 and pattern[i+7] == 1 and dots..settings.both_sym or
		pattern[i] == 1 and pattern[i+7] == 0 and dots..settings.top_sym or
		pattern[i] == 0 and pattern[i+7] == 1 and dots..settings.low_sym or
		dots..settings.empty_sym

		dots_s = pattern[i+14] == 1 and dots_s..settings.low_sym or
		dots_s..settings.empty_sym
	end
	--
	if not settings.show_seconds then
		statusd.inform("binclock", dots)
		statusd.inform("binclock_hint", settings.color)
	else
		statusd.inform("binclock", settings.l_border..dots..settings.separator..--
		dots_s..settings.r_border)
		statusd.inform("binclock_hint", settings.color)
	end
end


local function inform_ceros()
	if not settings.show_seconds then
		statusd.inform("binclock_10", table.concat(pattern, "", 3, 14))
		statusd.inform("binclock_10_hint", settings.color)
	else
		statusd.inform("binclock_10", table.concat(pattern, "", 3))
		statusd.inform("binclock_10_hint", settings.color)
	end
end


local function update_binary()
	convert_time()
	inform_ceros()
	inform_dots()
	binary_timer:set(settings.update_interval, update_binary)
end

binary_timer = statusd.create_timer()
update_binary()

