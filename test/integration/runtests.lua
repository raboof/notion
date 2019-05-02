--# on Debian in lua-posix
local basedir = "../../"

print("[TESTING] Integration")

local success, posix = pcall(require, "posix")
if not success then
    print("[SKIP] posix module not found")
    os.exit(0)
end

function sleep(sec)
    os.execute("sleep " .. tonumber(sec))
end

function getTests(testset)
  return (io.popen("ls " .. testset .. "/*.lua"):lines())
end

print('Running tests against ' .. basedir .. 'notion/notion')
print 'Starting Xorg dummy...'

local xpid = posix.fork()
if (xpid == 0) then
    local result,errstr,errno = posix.exec("/usr/bin/Xorg", "-noreset", "+extension", "GLX", "+extension", "RANDR", "+extension", "RENDER", "-logfile", "./10.log", "-config", "./xorg.conf", ":7")
    print('Error replacing current process with Xorg dummy: ' .. errstr);
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
    local result,errstr,errno = posix.exec(basedir .. "notion/notion", "-noerrorlog", "-display", ":7")
    print('Error replacing current process with notion: ' .. errstr)
    os.exit(1)
  end

  sleep(2)

  for test in getTests(testset) do
    print('[TEST] ' .. test)
    local testoutputpipe = io.popen("cat " .. test .. " | DISPLAY=:7 " .. basedir .. "mod_notionflux/notionflux/notionflux")
    local testoutput = testoutputpipe:read("*a")
    if(testoutput ~= "\"ok\"\n") then
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
