--# on Debian in lua-posix

print("[TESTING] Integration")

local basedir = "../../"
local notion_binary = basedir .. "notion/notion"
local notionflux_binary = basedir .. "mod_notionflux/notionflux/notionflux"

local running_on_travis = os.getenv('TRAVIS')
if running_on_travis then
    print('TRAVIS detected')
end

local success, posix = pcall(require, "posix")
if not success then
    print("[ERROR] posix module not found")
    os.exit(1)
end

function sleep(sec)
    os.execute("sleep " .. tonumber(sec))
end

function getTests(testset)
  return (io.popen("ls " .. testset .. "/*.lua"):lines())
end

local function file_exists(name)
    local f=io.open(name,"r")
    if f~=nil then io.close(f) return true else return false end
end

local function find_in_path(file)
    local env_path = os.getenv("PATH")
    local paths = string.gmatch(env_path, "[^:]+")
    for path in paths do
        local fullfilename = path .. '/' .. file
        if file_exists(fullfilename) then
            return fullfilename
        end
    end
    return false
end

print('Running tests against ' .. notion_binary)

local xpid = posix.fork()
if (xpid == 0) then
    if running_on_travis then
        print('TRAVIS should have Xfvb running')
        os.exit(1)
    end
    local xorg_binary = find_in_path("Xorg")
    if xorg_binary == false then
        print('Error launching Xorg dummy, Xorg binary not found')
    end
    print('Starting Xorg dummy: ' .. xorg_binary)
    local result,errstr,errno = posix.exec(xorg_binary, "-noreset", "+extension", "GLX", "+extension", "RANDR", "+extension", "RENDER", "-logfile", "./99.log", "-config", "./xorg.conf", ":99")
    print('Error replacing current process with Xorg dummy: ' .. errstr)
    os.exit(1)
end

sleep(3)

local testsets = { 'basic_test', 'xinerama', 'xrandr' }
local errors = 0

for i,testset in ipairs(testsets) do
  posix.setenv('HOME', testset);

  os.execute("rm -rf " .. testset .. "/.notion/default-session--99")

  print('Starting notion in ./' .. testset .. '...')

  local notionpid = posix.fork()
  if (notionpid == 0) then
    local result,errstr,errno = posix.exec(
        notion_binary,
        "-noerrorlog",
        "-s", basedir .. "contrib/scripts",
        "-s", basedir .. "de",
        "-s", basedir .. "etc",
        "-s", basedir .. "ioncore",
        "-s", basedir .. "mod_dock",
        "-s", basedir .. "mod_menu",
        "-s", basedir .. "mod_mgmtmode",
        "-s", basedir .. "mod_notionflux",
        "-s", basedir .. "mod_query",
        "-s", basedir .. "mod_sm",
        "-s", basedir .. "mod_sp",
        "-s", basedir .. "mod_statusbar",
        "-s", basedir .. "mod_statusbar/ion-statusd",
        "-s", basedir .. "mod_tiling",
        "-s", basedir .. "mod_xinerama",
        "-s", basedir .. "mod_xkbevents",
        "-s", basedir .. "mod_xrandr",
        "-display", ":99"
        )
    print('Error replacing current process with notion: ' .. errstr)
    os.exit(1)
  end

  sleep(3)

  for test in getTests(testset) do
    print('[TEST] ' .. test)
    local testoutputpipe = io.popen("cat " .. test .. " | DISPLAY=:99 " .. notionflux_binary)
    local testoutput = testoutputpipe:read("*a")
    local okend = "\"ok\"\n"
    if(testoutput == "" or testoutput:sub(-#okend) ~= okend) then
      print('[ERROR] ' .. testoutput)
      errors = errors + 1
    else
      print '[OK]'
    end
  end

  print('Killing notion process...')
  posix.kill(notionpid, posix.SIGKILL)
  print(posix.wait(notionpid))
  sleep(3)

end

print 'Killing X process...'
posix.kill(xpid)

if errors == 0 then
  print '[OK]'
else
  print('[ERROR] ' .. errors .. " tests failed.")
end
os.exit(errors)
