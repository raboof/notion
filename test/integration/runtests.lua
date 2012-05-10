require "posix"

function sleep(sec)
    os.execute("sleep " .. tonumber(sec))
end

print 'Running tests against currently *installed* Notion'
print 'Starting Xdummy...'

local xpid = posix.fork()
if (xpid == 0) then
    local result,errstr,errno = posix.exec("/bin/sh", "./Xdummy", ":7")
    print ('Error replacing current process with Xdummy: ' .. errstr)
    os.exit(1)
end

sleep(1)

print '(Hopefully) started Xdummy.'

print 'Starting notion...'
local notionpid = posix.fork()
if (notionpid == 0) then
    local result,errstr,errno = posix.exec("../../notion/notion", "-display", ":7")
    print ('Error replacing current process with notion: ' .. errstr)
    os.exit(1)
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
