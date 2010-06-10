--
-- closeorkill.lua 
-- 
-- This script will first attempt to close a region and, if that fails on
-- a client window and this script is called again within MAX_DIFFTIME 
-- (5 secs) time, kill the application.
--
-- To use, change (when using the default bindings)
--   kpress_waitrel(DEFAULT_MOD.."C", close_sub_or_self)
-- in ioncore-bindings.lua to
--   kpress_waitrel(DEFAULT_MOD.."C", close_or_kill_sub_or_self)
--

local MAX_DIFFTIME=5

local last_tried, last_tried_time

function close_or_kill(reg)
    local time=os.time()
    if last_tried==reg and obj_is(reg, "WClientWin") then
        if os.difftime(time, last_tried_time)<=MAX_DIFFTIME then
            reg:kill()
            last_tried=nil
            return
        end
    end
    
    last_tried=reg
    last_tried_time=time
    reg:close()
end

function close_or_kill_sub_or_self(reg)
    close_or_kill(reg:active_sub() or reg)
end

