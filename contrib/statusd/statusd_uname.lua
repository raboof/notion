-- statusd_uname.lua
--[[
Example of how to use statusd.popen_bgread() with coroutines to avoid blocking
while reading I/O.

This only has one key to keep it simple: %uname_a

Normal Usage:

1) If you do not have ~/.ion3/cfg_statusbar.lua, copy that file from Ion3 to
   your ~/.ion3 directory.  On Debian, it is in /etc/X11/ion3/cfg_statusbar.lua.

2) Ion3 will load the appropriate modules if they are in the template at
   startup.

   So place '%uname_a' into the template in ~/.ion3/cfg_statusbar.lua.


Also, if you want to test this independent of ion, you can do this:

1) Copy statusd_uname.lua to ~/.ion3/

2) Run '/usr/lib/ion3/ion-statusd -m uname'
   This will dump out all of the updates to the terminal as they happen

3) Hit control+c when you are done testing.


License: Public domain

tyranix [ tyranix at gmail ]

--]]

local defaults={
    -- Update every minute
    update_interval=60*1000,
}

local uname_timer = nil
local settings=table.join(statusd.get_config("uname"), defaults)

-- Parse the output and then tell statusd when we have all of the value.
function parse_uname(partial_data)
    -- Keep reading partial_data until it returns nil
    local result = ""
    while partial_data do
        result = result .. partial_data
        -- statusd.popen_bgread() will resume us when it has more data
        partial_data = coroutine.yield()
    end

    -- If we have a new result, tell statusd
    if result and result ~= "" then
        statusd.inform("uname_a", result)
    end

    -- Setup the next execution.
    uname_timer:set(settings.update_interval, update_uname)
end

-- If we get any stderr, just print it out.
local function flush_stderr(partial_data)
    -- Continually grab stderr
    local result = ""
    while partial_data do
        result = result .. partial_data
        partial_data = coroutine.yield()
    end

    if result and result ~= "" then
        print("STDERR:", result, "\n")
    end
end

-- Query mocp to get information for statusd.
function update_uname()
    statusd.popen_bgread('uname -a',
                         coroutine.wrap(parse_uname),
                         coroutine.wrap(flush_stderr))
end

-- Timer so we can keep telling statusd what the current value is.
uname_timer = statusd.create_timer()
update_uname()

-- vim:tw=0:
