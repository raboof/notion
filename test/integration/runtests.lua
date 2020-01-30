--# on Debian in lua-posix
local basedir = "../../"

print("[TESTING] Integration")

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

print('Running tests against ' .. basedir .. 'notion/notion')

local xpid = posix.fork()
if (xpid == 0) then
    if os.getenv('TRAVIS') == true then
        print('TRAVIS should have Xfvb running')
        os.exit(1)
    end
    local xorg_binary = find_in_path("Xorg")
    if xorg_binary == false then
        print('Error launching Xorg dummy, Xorg binary not found')
    end
    print('Starting Xorg dummy: ' .. xorg_binary)
    local result,errstr,errno = posix.exec(xorg_binary, "-noreset", "+extension", "GLX", "+extension", "RANDR", "+extension", "RENDER", "-logfile", "./10.log", "-config", "./xorg.conf", ":99")
    print('Error replacing current process with Xorg dummy: ' .. errstr)
    os.exit(1)
end

sleep(1)

local testsets = { 'basic_test', 'xinerama', 'xrandr' }
local errors = 0

for i,testset in ipairs(testsets) do
  posix.setenv('HOME', testset);

  os.execute("rm -rf " .. testset .. "/.notion/default-session--7")

  print('Starting notion in ./' .. testset .. '...')

  local notionpid = posix.fork()
  if (notionpid == 0) then
    local result,errstr,errno = posix.exec(basedir .. "notion/notion", "-noerrorlog", "-display", ":99")
    print('Error replacing current process with notion: ' .. errstr)
    os.exit(1)
  end

  sleep(2)

  for test in getTests(testset) do
    print('[TEST] ' .. test)
    local testoutputpipe = io.popen("cat " .. test .. " | DISPLAY=:99 " .. basedir .. "mod_notionflux/notionflux/notionflux")
    local testoutput = testoutputpipe:read("*a")
    local okend = "\"ok\"\n"
    if(testoutput == "" or testoutput:sub(-#okend) ~= okend) then
      print('[ERROR] ' .. testoutput)
      errors = errors + 1
    else
      print '[OK]'
    end
  end

  print 'Killing notion process...'
  posix.kill(notionpid)
  sleep(1)

end

print 'Killing X process...'
posix.kill(xpid)

if errors == 0 then
  print '[OK]'
else
  print('[ERROR] ' .. errors .. " tests failed.")
end
os.exit(errors)
