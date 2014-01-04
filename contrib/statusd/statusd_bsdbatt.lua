-- Authors: Russell A. Jackson
-- License: BSD 2-Clause License, see http://opensource.org/licenses/bsd-license.php
-- Last Changed: 2005
--
-- Copyright (c) 2005 Russell A. Jackson
-- All rights reserved.
--
-- Redistribution and use in source and binary forms, with or without
-- modification, are permitted provided that the following conditions
-- are met:
-- 1. Redistributions of source code must retain the above copyright
--    notice, this list of conditions and the following disclaimer.
-- 2. Redistributions in binary form must reproduce the above copyright
--    notice, this list of conditions and the following disclaimer in the
--    documentation and/or other materials provided with the distribution.
--
-- THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
-- ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
-- IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
-- ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
-- FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
-- DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
-- OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
-- HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
-- LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
-- OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
-- SUCH DAMAGE.
--
-- statusd_bsdbatt.lua
-- 
-- Uses sysctl interface to fetch ACPI CMBAT status on FreeBSD.
-- Add bsdbatt_life and bsdbatt_time fields to status bar template.
--

function get_bsdbatt()
	local f=io.popen('sysctl -n hw.acpi.battery.life', 'r')
	local life=f:read('*l')
	f:close()

	f=io.popen('sysctl -n hw.acpi.battery.time', 'r')
	local time=f:read('*l')
	f:close()

	life = life.."%"

	if time == "-1" then
		time = "AC"
	else
		time = time.."m"
	end

	return life, time
end

function update_bsdbatt()
	local life, time = get_bsdbatt()

	statusd.inform("bsdbatt_life", life)
	statusd.inform("bsdbatt_time", time)

	bsdbatt_timer:set(1000, update_bsdbatt)
end

bsdbatt_timer = statusd.create_timer()
update_bsdbatt()
