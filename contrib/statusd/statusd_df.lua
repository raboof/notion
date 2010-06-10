-- $Id: statusd_df.lua 60 2006-11-14 11:19:29Z tibi $

-- statusd_df.lua -- disk space monitor for Ion3's statusbar

-- version : 0.1
-- date    : 2006-11-14
-- author  : Tibor Csögör <tibi@tiborius.net>

-- Shows the disk space usage of given file systems.

-- The standard UNIX tool df(1) is used to obtain the needed data.  This script
-- assumes that df understands the -k flag and produces an output like the
-- following (the header line is ignored):
-- Filesystem           1K-blocks      Used Available Use% Mounted on
-- /dev/hda6              2403420     33264   2248064   2% /tmp

-- Configuration:

-- The `template' string controls how the output will be formated.  It may
-- contain the following keywords:

-- PLACEHOLDER	DESCRIPTION		EXAMPLE
-- -----------	-----------		-------
-- mpoint	mount point		/home
-- fs		file system		/dev/sda2
-- size		size of fs		40G
-- used		used space		12.6G
-- usedp	used space in %		31.5%
-- avail	available space		27.4G
-- availp	available space in %	68.5%

-- The `fslist' holds the file systems (mount points) to be monitored.
-- `separator' is placed between the entries in the output.  Statistics are
-- updated every `update_interval' milliseconds.

-- This software is in the public domain.

--------------------------------------------------------------------------------


local defaults = {
   template = "[%mpoint: %avail (%availp) free]",
   fslist = { "/" },
   separator = "  ",
   update_interval = 1000, -- 1 second
}
                
local settings = table.join(statusd.get_config("df"), defaults)

local df_timer = statusd.create_timer()

function math.round(num, idp)
   local mult = 10^(idp or 0)
   return math.floor(num  * mult + 0.5) / mult
end

local function guess_mem_unit(amount)
   amount = tonumber(amount)
   if (amount < 1024) then
      return amount .. "k"
   elseif (amount >= 1024) and (amount < 1048576) then
      return math.round((amount / 1024), 0) .. "M"
   elseif (amount > 1048576) then
      return math.round((amount / 1048576), 1) .. "G"
   end
end

local function get_df()
   local df_table = {}
   local f = io.popen('df -k', 'r')
   if (f == nil) then return nil end
   f:read("*line") -- skip header line
   local s = f:read("*a")
   f:close()
   local i = 0
   while (i < string.len(s)) do
      local j, fsname, fssize, fsused, fsavail, fsusedp, mpoint
      i, j, fsname, fssize, fsused, fsavail, fsusedp, mpoint
	 = string.find(s, "(/%S+)%s+(%d+)%s+(%d+)%s+(%d+)%s+(%d+)%%?%s(%S+)\n",
		       i)
      if (i == nil) then return nil end
      df_table[mpoint] = { mpoint=mpoint,
	                   fs=fsname,
	                   size=guess_mem_unit(tonumber(fssize)),
			   used=guess_mem_unit(tonumber(fsused)),
			   avail=guess_mem_unit(tonumber(fsavail)),
			   usedp=tonumber(fsusedp),
			   availp=((100 - tonumber(fsusedp)) .. "%") }
      i = j+1
   end
   return df_table
end

local function update_df()
   local t = get_df()
   if (t == nil) then return nil end
   local df_str = ""
   for i=1, #settings.fslist do
      local s = string.gsub(settings.template, "%%(%w+)",
			    function (arg)
			       if (t[settings.fslist[i]] ~= nil) then
				  return t[settings.fslist[i]][arg]
			       end
			       return nil
			    end)
      df_str = df_str .. settings.separator .. s
   end
   df_str = string.sub(df_str, #settings.separator + 1)
   statusd.inform("df", df_str)
   df_timer:set(settings.update_interval, update_df)
end

update_df()

-- EOF
