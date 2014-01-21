-- Authors: Unknown
-- License: Unknown
-- Last Changed: Unknown
-- 
-- Creates scratchpads on a per-frame/per-clientwin basis.

-- I realized that sometimes it might be useful to have a scratchpad for
-- notes and things for only one frame/cwin, and not globally.

-- This script creates two bindings:
--   CTRL-"space" to create/toggle per-frame scratchpads
-- and
--   CTRL-META-"space" to create/toggle per-cwin scratchpads

-- The per-frame sps are smarter and won't stay showing across desktops, the
-- per-cwin sps are dumber and will.

local cwinsps = nil
local cwinsps_rev = nil
local framesps = nil
local framesps_rev = nil

--{{{ load_sps

local function load_sps()
    local cwint = ioncore.read_savefile("cwin_sps")
    local framet = ioncore.read_savefile("frame_sps")

--{{{ Tuomo does this in check_and_create in mod_sp so I can do it here
    local hook

    hook = ioncore.get_hook("ioncore_post_layout_setup_hook")
    if hook then
	hook:remove(load_sps)
    end
--}}}


-- Initialize these here so I can test for nil to see if I've done my startup stuff
    cwinsps = setmetatable({}, {__mode="kv"})
    cwinsps_rev = setmetatable({}, {__mode="kv"})
    framesps = setmetatable({}, {__mode="kv"})
    framesps_rev = setmetatable({}, {__mode="kv"})

    if cwint then
	for k,v in pairs(cwint) do
	    local reg, sp

	    reg = ioncore.lookup_region(k)
	    sp = ioncore.lookup_region(v)

	    if reg and sp then
		cwinsps[reg] = sp
		cwinsps_rev[sp] = reg
	    end
	end
    end

    if framet then
	for k,v in pairs(framet) do
	    local reg, sp

	    reg = ioncore.lookup_region(k)
	    sp = ioncore.lookup_region(v)

	    if reg and sp then
		framesps[reg] = sp
		framesps_rev[sp] = reg
	    end
	end
    end
end

--}}} load_sps

--{{{ save_sps

local function save_sps()
    local t = {}

    for k,v in pairs(cwinsps) do
	if obj_exists(k) and obj_exists(v) then
	    t[k:name()] = v:name()
	end
    end
    ioncore.write_savefile("cwin_sps", t)

    t = {}
    for k,v in pairs(framesps) do
	if obj_exists(k) and obj_exists(v) then
	    t[k:name()] = v:name()
	end
    end
    ioncore.write_savefile("frame_sps", t)
end

--}}} save_sps

--{{{ toggle_cwin_sp

function toggle_cwin_sp(parent, cwin)
    local g = {}
    local cg = {}
    local pg = {}
    local sp = nil

    if not cwinsps then
	load_sps()
    end

    if cwin then
	sp = cwinsps[cwin]
	pg = parent:geom()
	cg = cwin:geom()

	g.x = pg.x + cg.x
	g.y = pg.y + cg.y
	g.w = cg.w
	g.h = cg.h
    end

    if not sp or not obj_exists(sp) then
	local sp_rev = cwinsps_rev[parent]

	if sp_rev then
	    mod_sp.set_shown(parent, 'toggle')
	else
	    local scr = cwin:screen_of()
	    local name = cwin:name().."_cwin_sp"

	    sp = scr:attach_new({type = 'WFrame', layer = 2,
	                         name = name, sizepolicy = 3,
	                         frame_style = 'frame-scratchpad',
	                         geom = g})

	    cwinsps[cwin] = sp
	    cwinsps_rev[sp] = cwin
	end
    else
	mod_sp.set_shown(sp, 'toggle')
    end
end

--}}} toggle_cwin_sp

--{{{ toggle_frame_sp

function toggle_frame_sp(frame)
    local sp = nil

    if not framesps then
	load_sps()
    end

    sp = framesps[frame]

    if (not sp or not obj_exists(sp)) then
	local sp_rev = framesps_rev[frame]

	if sp_rev then
	    mod_sp.set_shown(frame, 'toggle')
	else
	    local name = frame:name().."_frame_sp"

	    sp = frame:attach_new({type = 'WFrame', layer = 2,
	                           name = name, sizepolicy = 3,
	                           frame_style = 'frame-scratchpad'})

	    framesps[frame] = sp
	    framesps_rev[sp] = frame
	end
    else
	mod_sp.set_shown(sp, 'toggle')
    end
end

--}}} toggle_frame_sp

--{{{ Init

local function setup_hooks()
    local hook

    hook = ioncore.get_hook("ioncore_post_layout_setup_hook")
    if hook then
	hook:add(load_sps)
    end

    hook = ioncore.get_hook("ioncore_snapshot_hook")
    if hook then
	hook:add(save_sps)
    end
end

--}}} Init

--{{{ Bindings

defbindings("WFrame", {
--    kpress(CTRL.."space", "toggle_frame_sp(_)"),
--    kpress(CTRL..META.."space", "toggle_cwin_sp(_, _sub)"),
    kpress("Control+space", "toggle_frame_sp(_)"),
    kpress(META.."Control+space", "toggle_cwin_sp(_, _sub)"),
})

--}}}

setup_hooks()

-- vim:foldmethod=marker
