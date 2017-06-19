-- Authors: Randall Wald <randy@rwald.com>
-- License: GPL, version 2
-- Last Changed: Unknown
--
-- statusd_info.lua
-- CPU, Mem, and Swap information script
-- Written by Randall Wald
-- email: randy@rwald.com
-- Released under the GPL
--
-- This script is based on parsing 'top' output.
--
-- Unfortunately top output is inconsistent among versions. Some versions
-- (such as 3.2.8) are known not to work, as they do not support the 'b'
-- mode. Other versions do not work correctly with '-n 1', so we pass '-n 2'
-- just to be sure.
--
-- We currently recognise 2 output formats for the command
--     top b -n 2 -d 1 -p 0|grep Cpu|tail -n 1
--
-- Cpu(s): 16.9% us,  5.1% sy,  0.0% ni, 70.8% id,  6.5% wa,  0.1% hi,  0.5% si
-- %Cpu(s):  4.5 us,  1.0 sy,  0.0 ni, 93.5 id,  1.0 wa,  0.0 hi,  0.0 si,  0.0 st
--
-- Let us know when you encounter another variation, perhaps we can support it, too.
--
-- Available monitors:
-- 	%info_CPU_user		Percentage of CPU used by user programs
-- 	%info_CPU_system	Percentage of CPU used by services
-- 	%info_CPU_idle		Percentage of CPU idle
-- 	%info_CPU_ni		The time the CPU has spent running users'’processes that have been niced.
-- 	%info_CPU_wa		Amount of time the CPU has been waiting for I/O to complete.
-- 	%info_CPU_hi		The amount of time the CPU has been servicing hardware interrupts.
-- 	%info_CPU_si		The amount of time the CPU has been servicing software interrupts.
-- 	%info_RAM_total		Total amount of RAM
-- 	%info_RAM_used		Amount of RAM used
-- 	%info_RAM_free		Amount of RAM free
-- 	%info_RAM_shared	Amount of RAM shared
-- 	%info_RAM_buffers	Amount of RAM in buffers
-- 	%info_RAM_cached	Amount of RAM cached
-- 	%info_swap_total	Total amount of swap
-- 	%info_swap_used		Amount of swap currently used
-- 	%info_swap_free		Amount of swap currently free
--
-- Update Interval:
-- (Note that the units are milliseconds)

local update_interval = 0.1 * 1000

-- Memory monitors need a factor:
-- b - ""
-- k - "K"
-- m - "M"
-- g - "G"
local mem_dimension = "M"

-- Defines the factor for dividing the memory amount
if mem_dimension == "" then
	mem_factor = 1
elseif mem_dimension == "K" then
	mem_factor = 1024
elseif mem_dimension == "M" then
	mem_factor = 1024^2
else
	mem_factor = 1024^3
end

local function get_CPU_info()
	local f=io.popen('top b -n 2 -d 1 -p 0|grep Cpu|tail -n 1','r')
	local s=f:read('*all')
	f:close()
	local _, _,
		info_CPU_user,
		info_CPU_system,
		info_CPU_ni,
		info_CPU_idle,
		info_CPU_wa,
		info_CPU_hi,
		info_CPU_si = string.find(s, "Cpu%(s%):%s*(%d+%.%d+%%?)%s*us,%s*(%d+%.%d+%%?)%s*sy,%s*(%d+%.%d+%%?)%s*ni,%s*(%d+%.%d+%%?)%s*id,%s*(%d+%.%d+%%?)%s*wa,%s*(%d+%.%d+%%?)%s*hi,%s*(%d+%.%d+%%?)%s*si")
	return info_CPU_user.."", info_CPU_system.."", info_CPU_ni.."", info_CPU_idle.."", info_CPU_wa.."", info_CPU_hi.."", info_CPU_si..""
end

local function process_memory(value)
	local memory = value / mem_factor
-- Truncate to just two digits after the decimal place
	memory = string.gsub(memory,"(%d+%.%d%d)(%d*)","%1")
	return memory
end

local function get_RAM_info()
	local f=io.popen('free -b','r')
	local s=f:read('*all')
	f:close()
	local _, _,
		info_RAM_total,
		info_RAM_used,
		info_RAM_free,
		info_RAM_shared,
		info_RAM_buffers,
		info_RAM_cached = string.find(s, "Mem:%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)")
	info_RAM_total = process_memory(info_RAM_total)
	info_RAM_used = process_memory(info_RAM_used)
	info_RAM_free = process_memory(info_RAM_free)
	info_RAM_shared = process_memory(info_RAM_shared)
	info_RAM_buffers = process_memory(info_RAM_buffers)
	info_RAM_cached = process_memory(info_RAM_cached)
	local _, _,
		info_swap_total,
		info_swap_used,
		info_swap_free = string.find(s, "Swap:%s+(%d+)%s+(%d+)%s+(%d+)")
	info_swap_total = process_memory(info_swap_total)
	info_swap_used = process_memory(info_swap_used)
	info_swap_free = process_memory(info_swap_free)
	return info_RAM_total..mem_dimension, info_RAM_used..mem_dimension, info_RAM_free..mem_dimension, info_RAM_shared..mem_dimension, info_RAM_buffers..mem_dimension, info_RAM_cached..mem_dimension, info_swap_total..mem_dimension, info_swap_used..mem_dimension, info_swap_free..mem_dimension
end

local function inform_info(name, value)
	if statusd ~= nil then
		statusd.inform(name, value)
	else
		io.stdout:write(name..": "..value.."\n")
	end
end

if statusd ~= nil then
	status_timer = statusd.create_timer()
end

local function update_info()
	local info_CPU_user, info_CPU_system, info_CPU_ni, info_CPU_idle, info_CPU_wa, info_CPU_hi, info_CPU_si = get_CPU_info()
	local info_RAM_total, info_RAM_used, info_RAM_free, info_RAM_shared, info_RAM_buffers, info_RAM_cached, info_swap_total, info_swap_used, info_swap_free = get_RAM_info()
	inform_info("info_CPU_user", info_CPU_user)
	inform_info("info_CPU_system", info_CPU_system)
	inform_info("info_CPU_ni", info_CPU_ni)
	inform_info("info_CPU_idle", info_CPU_idle)
	inform_info("info_CPU_wa", info_CPU_wa)
	inform_info("info_CPU_hi", info_CPU_hi)
	inform_info("info_CPU_si", info_CPU_si)
	inform_info("info_RAM_total", info_RAM_total)
	inform_info("info_RAM_used", info_RAM_used)
	inform_info("info_RAM_free", info_RAM_free)
	inform_info("info_RAM_shared", info_RAM_shared)
	inform_info("info_RAM_buffers", info_RAM_buffers)
	inform_info("info_RAM_cached", info_RAM_cached)
	inform_info("info_swap_total", info_swap_total)
	inform_info("info_swap_used", info_swap_used)
	inform_info("info_swap_free", info_swap_free)
	if statusd ~= nil then
		status_timer:set(update_interval, update_info)
	end
end

update_info()
