-- Authors: <delirium@hackish.org>
-- License: Unknown
-- Last Changed: Unknown
--
-- statusd for MPD (Music Player Daemon)
-- bugs/requests/comments: delirium@hackish.org

-- requires that netcat is available in the path

local defaults={
    -- 500 or less makes seconds increment relatively smoothly while playing
    update_interval=500,

    -- mpd server info (localhost:6600 are mpd defaults)
    address="localhost",
    port=6600,

    -- mpd password (if any)
    password=nil,

    -- display template
    -- ---
    -- can use the following:
    --   track metadata: %artist, %title, %num, %album, %year, %len
    --   current track position: %pos
    --   escape for the percent character: %%

    -- a default template
    template = "%artist - %num - %title (%pos / %len)"
}

local settings=table.join(statusd.get_config("mpd"), defaults)

local success
local last_success

local function saferead(file)
	local data, err, errno
	repeat
		data, err, errno = file:read()
	until errno ~= 4 -- EINTR
	return data, err, errno
end

local function get_mpd_status()

    -- The first version of cmd_string is the original, however, some versions
    -- of MPD close the socket too quickly if close is included. If you see the
    -- message: error querying mpd status
    -- then try the second version.
    local cmd_string = "status\ncurrentsong\nclose\n"
--    local cmd_string = "status\ncurrentsong\n"

    if settings.password ~= nil then
        cmd_string = "password " .. settings.password .. "\n" .. cmd_string
    end
    cmd_string = string.format('echo -n "%s" | netcat %s %d',
                               cmd_string, settings.address, settings.port)

    last_success = success
    success = false

    local mpd = io.popen(cmd_string, "r")

    -- welcome msg
    local data = saferead(mpd)
    if data == nil or string.sub(data,1,6) ~= "OK MPD" then
	mpd:close()
        return "mpd not running"
    end

    -- 'password' response (if necessary)
    if settings.password ~= nil then
        repeat
            data = saferead(mpd)
        until data == nil or string.sub(data,1,2) == "OK" or string.sub(data,1,3) == "ACK"
        if data == nil or string.sub(data,1,2) ~= "OK" then
	    mpd:close()
            return "bad mpd password"
        end
    end

    local info = {}

    -- 'status' response
    repeat
        data = saferead(mpd)
        if data == nil then break end
	
        local _,_,attrib,val = string.find(data, "(.-): (.*)")
        if attrib == "time" then
            _,_,info.pos,info.len = string.find(val, "(%d+):(%d+)")

	    -- Around Lua 5.1, math.mod() was renamed math.fmod().
	    if type(math.mod) == "function" then
               info.pos = string.format("%d:%02d", math.floor(info.pos / 60), math.mod(info.pos, 60))
               info.len = string.format("%d:%02d", math.floor(info.len / 60), math.mod(info.len, 60))
	    else
               info.pos = string.format("%d:%02d", math.floor(info.pos / 60), math.fmod(info.pos, 60))
               info.len = string.format("%d:%02d", math.floor(info.len / 60), math.fmod(info.len, 60))
	    end
        elseif attrib == "state" then
            info.state = val
        end
    until string.sub(data,1,2) == "OK" or string.sub(data,1,3) == "ACK"
    if data == nil or string.sub(data,1,2) ~= "OK" then
	mpd:close()
        return "error querying mpd status"
    end

    -- 'currentsong' response
    repeat
        data = saferead(mpd)
        if data == nil then break end

        local _,_,attrib,val = string.find(data, "(.-): (.*)")
        if     attrib == "Artist" then info.artist = val
        elseif attrib == "Title"  then info.title  = val
        elseif attrib == "Album"  then info.album  = val
        elseif attrib == "Track"  then info.num    = val
        elseif attrib == "Date"   then info.year   = val
        end
    until string.sub(data,1,2) == "OK" or string.sub(data,1,3) == "ACK"
    if data == nil or string.sub(data,1,2) ~= "OK" then
	mpd:close()
        return "error querying current song"
    end

    mpd:close()

    success = true

    -- done querying; now build the string
    if info.state == "play" then
        local mpd_st = settings.template
        -- fill in %values
        mpd_st = string.gsub(mpd_st, "%%([%w%_]+)", function (x) return(info[x]  or "") end)
        mpd_st = string.gsub(mpd_st, "%%%%", "%%")
        return mpd_st
    elseif info.state == "pause" then
        return "Paused"
    else
        return "No song playing"
    end
end


local mpd_timer

local function update_mpd()
    -- update unless there's an error that's not yet twice in a row, to allow
    -- for transient errors due to load spikes
    local mpd_st = get_mpd_status()
    if success or not last_success then
        statusd.inform("mpd", mpd_st)
    end
    mpd_timer:set(settings.update_interval, update_mpd)
end

-- Init
mpd_timer=statusd.create_timer()
update_mpd()

