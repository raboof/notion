-- Authors: Jakub Ružička <yac@email.cz>
-- License: Unknown
-- Last Changed: 2008-08-14
--
-- statusd_mcpu.lua - multi CPU monitor for ion3 statusbar
--
-- version 0.1 by Jakub Ružička <yac@email.cz>, 2008-08-14
--
--
-- Small and hopefully efficient monitor for multiple CPU using Linux
-- /proc/stat.
--
-- in cfg_statusbar.lua you can
--
--   use following meters:
--     %mcpu - avearge load of all CPUs
--     %mcpu_n - average load of n-th CPU (zero-indexed - %mcpu_0, %mcpu_1 etc.)
--
--   set options:
--     interval - update interval
--     prc_char - percent character (may be '')
--     important_threshold - important hint above this % ( >100 to disable )
--     critical_threshold - critical hint above this % ( >100 to disable )
--
-- for example you can put the following in your template for dual core system:
--     template = ' cpu: %mcpu [ %mcpu_0, %mcpu_1 ] '
--
-- NOTES:
--
-- * It's very easy to add avg and per-cpu  user, nice, system, idle, iowait, irq
-- and softirg meters using a_* values as described in update_mcpu() but I
-- don't want/need these so you have to add them yourself.
--
-- * Script will (or should) work for one CPU but will produce mcpu and
-- redundant mcpu_0. If you want it to be efficient on one CPU, just edit it or
-- if you think the script should take care of it(extra ifs... pfff), mail me.
--

local defaults = {
  interval = 1000,
  prc_char = '%',
  important_threshold = 60,
  critical_threshold = 90,
}

local settings = table.join(statusd.get_config("mcpu"), defaults)

-- how many CPUs?
local f=io.popen("cat /proc/stat | grep '^cpu' | wc -l","r")
if not f then
  print "Failed to find out number of CPUs."
  return 0
end
local cpus = tonumber( f:read('*a') ) - 1
f:close()

-- ugly global init :o]
user, nice, system, idle, iowait, irq, softirq = {}, {}, {}, {}, {}, {}, {}
a_user, a_nice, a_system, a_idle, a_iowait, a_irq, a_softirq = {}, {}, {}, {}, {}, {}, {}
for i = 0,cpus do
  user[i], nice[i], system[i], idle[i], iowait[i], irq[i], softirq[i] = 0, 0, 0, 0, 0, 0, 0
  a_user[i], a_nice[i], a_system[i], a_idle[i], a_iowait[i], a_irq[i], a_softirq[i] = 0, 0, 0, 0, 0, 0, 0
end

-- refresh values
function mcpu_refresh()
  local i=0
  -- not sure if breaking this cycle closes file automaticaly as it should...
  for line in io.lines( '/proc/stat' ) do

    suc, _, n_user, n_nice, n_system, n_idle, n_iowait, n_irq, n_softirq = string.find( line,
      '^cpu%d?%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+%d+%s+%d+%s*$' )

    if suc then
      -- current values --
      a_user[i], a_nice[i], a_system[i], a_idle[i], a_iowait[i], a_irq[i], a_softirq[i] =
        n_user - user[i], n_nice - nice[i], n_system - system[i], n_idle - idle[i], n_iowait - iowait[i], n_irq - irq[i], n_softirq - softirq[i]

      -- last --
      user[i], nice[i], system[i], idle[i], iowait[i], irq[i], softirq[i] = n_user, n_nice, n_system, n_idle, n_iowait, n_irq, n_softirq
    else
      if i < cpus then
        -- this will probably suck on single CPU systems
        print( string.format( "Getting CPU info for all CPUs failed. (i=%d,cpus=%d)", i, cpus) )
        break
      end
    end

    i = i + 1
  end
end

--- main ---

local mcpu_timer = statusd.create_timer()

local function update_mcpu()
  local all
  local prc = {}, lprc

  mcpu_refresh()

  for i = 0,cpus do
    all = a_user[i] + a_nice[i] + a_system[i] + a_idle[i] + a_iowait[i] + a_irq[i] + a_softirq[i]
    prc[i] = math.floor( (1.0 - a_idle[i]/all ) * 100 + 0.5 )
  end

  -- summary for all CPUs
  lprc = prc[0]
  statusd.inform('mcpu', lprc..settings.prc_char )
  -- you can put here something like:
  -- statusd.inform('mcpu_user', a_user[0]..settings.prc_char )

  -- hint
  if lprc >= settings.critical_threshold then
    statusd.inform( "mcpu_hint", "critical" )
  elseif lprc >= settings.important_threshold then
    statusd.inform( "mcpu_hint", "important" )
  else
    statusd.inform( "mcpu_hint", "normal" )
  end

  -- each CPU (zero indexed)
  for i = 1,cpus do
    lprc = prc[i]
    statusd.inform('mcpu_'..(i-1), lprc..settings.prc_char )
    -- again, this is possible:
    -- statusd.inform('mcpu_'..(i-1)..'_user', a_user[i]..settings.prc_char )

    -- hint
    local hint='mcpu_'..(i-1)..'_hint'
    if lprc >= settings.critical_threshold then
      statusd.inform( hint, "critical" )
    elseif lprc >= settings.important_threshold then
      statusd.inform( hint, "important" )
    else
      statusd.inform( hint, "normal" )
    end
  end

  mcpu_timer:set(settings.interval, update_mcpu)
end

update_mcpu()
