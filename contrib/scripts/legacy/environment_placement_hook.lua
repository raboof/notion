-- Authors: Unknown
-- License: Unknown
-- Last Changed: Unknown
--
-- environment_placement_hook.lua
--
-- Linux-only placement hook which detects the presence of an ION_USE_WS
-- environment variable in the processes of new windows, and uses that to
-- determine where to place them.
--
-- ION_USE_WS should be exported in the same terminal the app is started
-- from, or something like this could be typed at the "Run:" prompt:
--
--     export ION_USE_WS="framename" && firefox
--
-- Or simply use the run_here query provided to "Run (in current frame):"
--
-- Note: there are a lot of apps that this will not work with.
--

defbindings("WMPlex", {
    kpress("Shift+F3", "run_here(_)")
})

local pid_prop_atom = ioncore.x_intern_atom("_NET_WM_PID", false)
local function pidof(w)
    return ioncore.x_get_window_property(w:xid(), pid_prop_atom, 0, 0, true)[1]
end

local function get_environment_of(pid)
    local file = io.open("/proc/"..tostring(pid).."/environ")
    local envstr = string.format("%q", file:read("*a"))

    file:close()

    local rv = {}
    for k, v in string.gmatch(envstr, "(.-)=(.-)\\000") do
	rv[k] = v
    end
    return rv
end


local function environ_placement_hook(w, t)
    if t.tfor then return false end -- ignore transients with this PID
    local pid = tostring(pidof(w))
    local env = get_environment_of(pid)
    local ws = env.ION_USE_WS

    if not ws then return false end
    ws = ioncore.lookup_region(ws)
    if not ws then return false end

    ws:attach(w, {switchto=true})
    return true
end

hook = ioncore.get_hook("clientwin_do_manage_alt")
if hook then
    hook:add(environ_placement_hook)
end

-- TODO: attempt to handle ":" for terminal apps.
local function qhandler(mplex, str)
    mod_query.exec_handler(mplex, "export ION_USE_WS="..
	string.shell_safe(mplex:name()).." && "..str)
end

function run_here(mplex)
    mod_query.query(mplex, "Run (in current frame):",
	nil, qhandler,
	mod_query.exec_completor, "run")
end
