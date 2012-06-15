require "posix"

function sleep(sec)
    os.execute("sleep " .. tonumber(sec))
end

function getTests(testset)
  return (io.popen("ls " .. testset .. "/*.lua"):lines())
end

print 'Running tests against ../../notion/notion'
print 'Starting Xdummy...'

local xpid = posix.fork()
if (xpid == 0) then
    local result,errstr,errno = posix.exec("/bin/sh", "./Xdummy", ":7")
    print ('Error replacing current process with Xdummy: ' .. errstr);
    os.exit(1)
end

sleep(1)

print '(Hopefully) started Xdummy.'

local testsets = { 'basic_test', 'xinerama' }
local errors = 0

for i,testset in ipairs(testsets) do

  posix.setenv('HOME', testset);

  print ('Starting notion in ./' .. testset .. '...')
  local notionpid = posix.fork()
  if (notionpid == 0) then
    local result,errstr,errno = posix.exec("../../notion/notion", "-noerrorlog", "-display", ":7")
    print ('Error replacing current process with notion: ' .. errstr)
    os.exit(1)
  end

  sleep(2)

  print 'Running tests...'

  for test in getTests(testset) do
    print ('Running test ' .. test)
    local testoutputpipe = io.popen("cat " .. test .. " | DISPLAY=:7 notionflux")
    local testoutput = testoutputpipe:read("*a")
    print 'Evaluating result...'
    if(testoutput ~= "\"ok\"\n") then
      print('** ERROR ** ' .. testoutput)
      errors = errors + 1
    else
      print '** OK **'
    end
  end

  print 'Killing notion process...'
  posix.kill(notionpid)

end

print 'Killing X process...'
posix.kill(xpid)

if errors == 0 then
  print 'OK!'
else
  print (errors .. " errors.")
end
