require "posix"

function sleep(sec)
    os.execute("sleep " .. tonumber(sec))
end

print 'Running tests against ../../notion/notion'
print 'Starting Xdummy...'

local xpid = posix.fork()
if (xpid == 0) then
    local errno = os.execute("/bin/sh ./Xdummy :7")
    if (errno ~= 0) then
      print ('Error starting Xdummy: ' .. errno)
      print ('Check your logs in /tmp/Xdummy.<username>/7/_var_log_Xorg.7.log')
      os.exit(1)
    else
      os.exit(0)
    end
end

sleep(1)

print '(Hopefully) started Xdummy.'

posix.setenv('HOME', './basic_test');

print 'Starting notion...'
local notionpid = posix.fork()
if (notionpid == 0) then
    local errno = os.execute("../../notion/notion -noerrorlog -display :7")
    if (errno ~= 0) then
      print ('Error starting notion: ' .. errno)
      os.exit(1)
    else
      os.exit(0)
    end
end

sleep(2)

print 'Running test...'
local testoutputpipe = io.popen("cat basic_test/01_call_notionflux.lua | DISPLAY=:7 notionflux")
local testoutput = testoutputpipe:read("*a")

print 'Killing processes...'
-- kill X and notion
posix.kill(notionpid)
posix.kill(xpid)

print 'Evaluating result...'
if(testoutput ~= "\"ok\"\n") then
    print('Error: ' .. testoutput)
    os.exit(1)
end

print 'OK!'
