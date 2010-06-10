--
-- By Jeremy Hankins <nowan@nowan.org>
--
-- Time-stamp: <2005-06-28 15:55:31 Jeremy Hankins>
--
-- statusd_dgs.lua: Check for moves waiting on www.dragongoserver.net
--
-- This monitor will check your status page on www.dragongoserver.net
-- for games waiting on a move.  Simply add %dgs to the statusd
-- template, and set a login and password.
--
-- In all honesty, I'd be surprised if there are more than a couple of
-- people who might use this script -- the intersection of go players
-- (and dgs users at that) and ion users can't be that large.
--
-- TO USE: Put %dgs in the template in cfg_statusbar.lua (e.g.,
-- "dgs:% %dgs") and  set the login and password.  Something like:
--
-- dgs={
--     login = "mylogin",
--     password = "ABCxyz",
-- },
--
-- in  mod_statusbar.launch_statusd.
--
-- NOTE: the wget command is used to fetch the status page and the
-- account info is passed on the command line.  This means that you need
-- wget for this monitor to work, and that your account info (including
-- password) will be available on your local system via ps.  If that
-- concerns you, don't use this script -- or provide a patch.  :)
--
-- BUG: Occasionally get_moves returns nil because it gets no output
-- from the wget.  This is either because I'm not using io.popen()
-- properly (I don't have docs for it) or, perhaps, a bug elsewhere.
-- When this happens you'll see a ? (with the critical hint set) instead
-- of a number for waiting moves.

if not statusd_dgs then
  statusd_dgs = {
    -- These should be set in cfg_statusbar:
    --login = "",
    --password = "",

    interval = 20*60*1000,  -- update every 20 minutes, 
    retry = 60 * 1000,      -- on error, retry in a minute

    -- These shouldn't need to be changed.
    login_url = "http://www.dragongoserver.net/login.php",
    --qick_url = "http://www.dragongoserver.net/quick_status.php"
  }
end

local timer
local meter = "dgs"

local settings = table.join(statusd.get_config(meter), statusd_dgs)

local check_cmd = "wget -qO - --post-data 'userid="..settings.login
    .."&passwd="..settings.password.."' '"..settings.login_url.."'"

if not settings.login or not settings.password then
    statusd.warn("You need to set a login and password.")
    return
end

-- 
-- get the move count
--
local function get_moves()
    local f = io.popen(check_cmd, 'r')
    if not f then return nil end

    local output = f:read('*a')
    f:close()
    if string.len(output) == 0 then return nil end

    local s, e, count = 0, 0, 0
    repeat
	s, e = string.find(output, 'href="game%.php%?gid=%d+"', e)
	if s then count = count + 1 end
    until not s

    return count
end

--
-- update the meter
--
local function update()
    local moves = get_moves()

    if moves then
	if moves > 0 then
	    statusd.inform(meter.."_hint", "important")
	else
	    statusd.inform(meter.."_hint", "normal")
	end
	statusd.inform(meter, tostring(moves))
	timer:set(settings.interval, update)
    else
	statusd.inform(meter.."_hint", "critical")
	statusd.inform(meter, "?")
	timer:set(settings.retry, update)
    end
end


timer = statusd.create_timer()
update()

