-- filename :   statusd_sysmon.lua
-- version  :   2.0
-- date     :   2005-03-29
-- 
-- system monitor for Ion3 WM statusbar
-- Written by Vladimir Chizhov < jagoterr *at* gmail *dot* com >
-- I'll apreciate any comments, bug reports and feature requests :)
--
-- Shows memory, swap and filesystem statistics, based on 'free' and 'df' commands output.
-- Allows lua-expression evaluation with the values.
--
-- This script can work not only as a Ion3 statusd monitor, but also as a standalone script.
-- If it works as a Ion statusd monitor, it reads user settings from statusd.

if not statusd_sysmon then
statusd_sysmon = {
    -- UPDATE INTERVAL
    -- 
    interval=10*1000,
    
    -- TEMPLATE STRING
    -- 
    -- COMMON:
    -- %%               - percents symbol (%)
    -- ${expression}    - evaluates lua expression
    --                    Shortly about expressions:
    --                      - may contain all macroses (of format %macros_name or
    --                      %{macros_name macros_params*})
    --                      - may contain global function calls
    --                      - may contain arithmetic, logical operations etc.
    --                      - there is a global function 'dpr' defined in this script
    --                      for decreasing double numbers' precision (see this
    --                      function comments for more information and the example templates)
    -- 
    -- RAM:
    -- %mem_total       - total available memory
    -- %mem_used        - used memory
    -- %mem_free        - free memory
    -- %mem_buffers     - buffered memory
    -- %mem_cached      - cached memory
    -- %mem_shared      - shared memory
    -- 
    -- SWAP:
    -- %swap_total       - total available swap space
    -- %swap_used        - used swap space
    -- %swap_free        - free swap space
    -- 
    -- FS (FileSystem):
    -- %{fs_total       mount_point}    - total available space on filesystem
    -- %{fs_used        mount_point}    - used space on filesystem
    -- %{fs_free        mount_point}    - free space on filesystem
    -- 
    -- TEMPLATE EXAMPLES:
    --
    -- simple swap statistics
    -- template = "swap used %swap_used of %swap_total"
    --
    -- used swap in percents with one sign after comma!
    -- how? so... try to understand, I think it's not very hard, if you read the previous comments
    -- template = "swap used by ${dpr (%swap_used / %swap_total * 100)}%%"
    --
    -- RAM used, excluding buffers and cached memory
    -- template = "RAM used: ${%mem_used - %mem_buffers - %mem_cached}"
    --
    -- root filesystem simple info
    -- note, that you should specify the actual mount point for filesystem, accordingly to /etc/mtab
    -- template = "/ used by %{fs_used /} of %{fs_total /}"
    --
    -- DEFAULT TEMPLATE:
    --
    template = "RAM: %mem_used / %mem_total MB (${dpr (%mem_used / %mem_total * 100, 1)} %%) * SWAP: %swap_used / %swap_total MB (${dpr (%swap_used / %swap_total * 100, 1)} %%) * /: %{fs_used /} / %{fs_total /} (${dpr (%{fs_used /} / %{fs_total /} * 100, 1)} %%)",

    -- DIMENSION for monitors
    -- 
    -- b - bytes
    -- k - kilobytes
    -- m - megabytes
    -- g - gigabytes
    -- 
    -- dimension for RAM and SWAP
    -- 
    mem_dimension   = "m",
    -- dimension for filesystems
    --
    fs_dimension    = "g",
}
end

local settings
if statusd ~= nil then
    settings = table.join (statusd.get_config ("sysmon"), statusd_sysmon)
else
    settings = statusd_sysmon
end

local factors = {
    b = 1,
    k = 1024,
    m = 1024^2,
    g = 1024^3,
}

local metrics = {}

-- --------------------------------------------------------------------------
-- Decreases the precision of the floating number
-- @param number    the number to decrease it's precision
-- @param signs_q   the quantity of signs after the comma to be left
function dpr (number, signs_q)
    local pattern = "%d+"
    if signs_q == nil then
        signs_q = 2
    end
    if signs_q ~= 0 then
        pattern = pattern.."%."
    end
    for i = 1, tonumber (signs_q)  do
        pattern = pattern.."%d"
    end
    return string.gsub (number, "("..pattern..")(.*)", "%1")
end

-- --------------------------------------------------------------------------
-- Retrieves information from 'free' command output
local function parse_free_command ()
    local f = io.popen ('free -'..settings.mem_dimension, 'r')
    local s = f:read('*a')
    f:close()
    local st, en
    
    st, en,
    metrics.mem_total,
    metrics.mem_used,
    metrics.mem_free,
    metrics.mem_shared,
    metrics.mem_buffers,
    metrics.mem_cached
        = string.find(s, 'Mem:%s*(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)')
    st, en,
    metrics.swap_total,
    metrics.swap_used,
    metrics.swap_free
        = string.find(s, 'Swap:%s*(%d+)%s+(%d+)%s+(%d+)')
end

-- --------------------------------------------------------------------------
-- Retrieves information from 'df' command output
local function parse_df_command ()
    local factor = factors[settings.fs_dimension]
    local f = io.popen ('df --block-size=1 -P', 'r')
    -- local s = f:read ("*l")
    for line in f:lines () do
        local st, en, fs, total, used, free, mount_point =
            string.find (line, '(%S+)%s+(%d+)%s+(%d+)%s+(%d+)%s+[%-%d]-%%?%s+(.*)')
        if fs ~= nil then
            metrics["fs_total->"..mount_point] = string.gsub (total / factor, '(%d+%.%d%d)(.*)', "%1")
            metrics["fs_used->"..mount_point] = string.gsub (used / factor, '(%d+%.%d%d)(.*)', "%1")
            metrics["fs_free->"..mount_point] = string.gsub (free / factor, '(%d+%.%d%d)(.*)', "%1")
        end
    end
    f:close ()

end

local sysmon_timer

-- --------------------------------------------------------------------------
-- Main calculating of values, template parsing and Ion statusd updating
local function update_sysmon ()
    local sysmon_st = settings.template
    
    parse_free_command ()

    parse_df_command ()
    
    -- filling the template by actual values
    sysmon_st = string.gsub (sysmon_st, "%%%{([%w%_]+)%s+(%S-)%}", function (arg1, arg2)
        return(metrics[arg1.."->"..arg2] or "")
    end)
    sysmon_st = string.gsub (sysmon_st, "%%([%w%_]+)", function (arg1)
        return (metrics[arg1] or "")
    end)
    sysmon_st = string.gsub (sysmon_st, "%$%{(.-)%}", function (arg1)
	return loadstring("return "..arg1)()
    end)
    -- replacing the '%%' macros with the '%' symbol
    sysmon_st = string.gsub (sysmon_st, "%%%%", "%%")

    if statusd ~= nil then
        statusd.inform ("sysmon_template", sysmon_st)
        statusd.inform ("sysmon", sysmon_st)
        sysmon_timer:set (settings.interval, update_sysmon)
    else
        io.stdout:write (sysmon_st.."\n")
    end
end

-- --------------------------------------------------------------------------
-- Init
if statusd ~= nil then
    sysmon_timer = statusd.create_timer ()
end
update_sysmon ()

