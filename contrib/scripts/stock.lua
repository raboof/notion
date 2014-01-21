-- Authors: Andrea Rossato <arossato@istitutocolli.org>
-- License: GPL, version 2 or later
-- Last Changed: 2006-07-10
-- 
-- stock.lua

-- ABOUT
-- An Ion3 applet for retrieving and displaying stock market information
-- from http://finance.yahoo.com. You can set up a portfolio and monitor
-- its intraday performance.


-- QUICKSTART
-- 1. In template of you cfg_statusbar.lua insert: "%stock" (without quotes)
-- 2. Insert, in you cfg_ion.lua or run: dopath("stock")
-- 3. press MOD1+F10 to get the menu
-- 4. Add a ticket: e.g. "^N225" (without quotes) to monitor the Nikkei index.  


-- COMMANDS
-- Here's the list of available commands:
--    - add-a-ticket: add a Yahoo ticket to monitor (e.g. "^N225" -
--      without quotes). You can also insert the quantity, separated by a
--      a comma: "TIT.MI,100" will insert 100 shares of TIT.MI in your portfolio.
--    - delete-a-ticket: remove a ticket
--    - suspend-updates: to stop retrieving data from Yahoo
--    - resume-updates: to resume retrieving data from Yahoo
--    - toggle-visibility: short or data display. You can configure
--      the string for the short display.
--    - update: force monitor to update data.

-- CONFIGURATION AND SETUP
-- You may configure this applet in cfg_statusbar.cfg 
-- In mod_statusbar.launch_statusd{) insert something like this:

-- -- stock configuration options
-- stock = {
--    tickets = {"^N225", "^SPMIB", "TIT.MI"},
--    interval = 5 * 60 * 1000,       -- check every 5 minutes
--    off_msg = "*Stock*",            -- string to be displayed in "short" mode
--    susp_msg = "(Stock suspended)", -- string to be displayed when data
--                                    -- retrieval is suspended
--    susp_msg_hint = "critical",     -- hint for suspended mode
--    unit = {
--       delta = "%",                 
--    },
--    important = {
--       delta = 0,
--    },
--    critical = {
--       delta = 0,
--    }
-- },

-- You can set "important" and "critical" thresholds for each meter.

-- PORTFOLIO
-- If you want to monitor a portfolio you can set it up in the configuration with 
-- something like this:
-- stock = {
--    off_msg = "*MyStock",	
--    portfolio = {
--       ["TIT.MI"] = 2000,
--       ["IBZL.MI"] = 1500,
--    },
-- },
-- where numbers represent the quantities of shares you posses. When
-- visibility will be set to OFF, in the statusbar (if you use ONLY the
-- %stock meter) you will get a string ("MyStock" in the above example)
-- red or green depending on the its global performance.

-- FEEDBACK
-- Please report your feedback, bugs reports, features request, to the
-- above email address.

-- REVISIONS
-- 2006-07-10 first release

-- LEGAL
-- Copyright (C) 2006  Andrea Rossato
 
-- This program is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License
-- as published by the Free Software Foundation; either version 2
-- of the License, or (at your option) any later version.

-- This software is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.

-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA


-- Have fun!

-- Andrea Rossato arossato AT istitutocolli DOT org


-- key bindings
-- you can (should) change the key bindings to your liking
defbindings("WMPlex", {
	       kpress(MOD1.."F10", "mod_query.query_menu(_, 'stockmenu', 'StockMonitor Menu: ')"),
	       kpress(MOD1.."Shift+F10", "StockMonitor.add_ticket(_)"),
	    })
defmenu("stockmenu", {
	   menuentry("update now",        "StockMonitor.update()"),
	   menuentry("add a ticket",      "StockMonitor.add_ticket(_)"),
	   menuentry("delete a Ticket",   "StockMonitor.del_ticket(_)"),
	   menuentry("toggle visibility", "StockMonitor.toggle()"),
	   menuentry("suspend updates",   "StockMonitor.suspend()"),
	   menuentry("resume updates",    "StockMonitor.resume()"),
	}
     )

-- our stuff
local function new_stock()
   local this = {
      --
      -- configuration
      --
      config = {
	 tickets = {},
	 interval = 5 * 60 * 1000,   -- check every 5 minutes
	 off_msg = "*Stock*",
	 susp_msg = "(Stock suspended)",
	 susp_msg_hint = "critical",
	 toggle = "on",
	 portfolio = {},
	 unit = {
	    delta = "%",
	 },
	 important = {
	    delta = 0,
	 },
	 critical = {
	    delta = 0,
	 }
      },
      -- end configuration

      status_timer = ioncore.create_timer(),
      url =  "http://finance.yahoo.com/d/quotes.csv",
      data = { },
      backup = { },
      paths = ioncore.get_paths(),
   }
   
   -- some needed global functions
   function table.merge(t1, t2)
      local t=table.copy(t1, false)
      for k, v in pairs(t2) do
	 t[k]=v
      end
      return t
   end

   function math.dpr(number, signs_q)
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

   -- gets configuration values and store them in this.config (public)
   function this.process_config()
      this.get_sb()
      local c = ioncore.read_savefile("cfg_statusd")
      if c.stock then 
	 this.config = table.merge(this.config, c.stock)
      end
      c = ioncore.read_savefile("cfg_stock")
      if c then 
	 this.config.portfolio = table.merge(this.config.portfolio, c)
      end
      this.process_portfolio()
   end
   
   -- gets tickets from portfolio
   function this.process_portfolio()
      for t,q in pairs(this.config.portfolio) do
	 if t then table.insert(this.config.tickets, t) end
      end
   end

   -- gets the statusbar obj and makes a backup of the sb table (just in case)
   function this.get_sb()
      for _, sb in pairs(mod_statusbar.statusbars()) do  
	 this.StatusBar = sb
      end
      ioncore.write_savefile("stock_sb", this.StatusBar:get_template_table())
   end
   
   -- removes a meter from the statusbar and inserts a new template chunk
   function this.sb_insert_tmpl_chunk(chunk, meter)
      local pos, old_sb_chunk
      this.restore_sb(meter)
      local old_sb = this.StatusBar:get_template_table()
      for a, item in pairs(old_sb) do
	 if item.meter == meter then 
	    pos = a
	    old_sb_chunk = item
	    break
	 end
      end
      this.backup[meter] = old_sb_chunk
      mod_statusbar.inform("start_insert_by_"..meter, "")
      mod_statusbar.inform("end_insert_by_"..meter, "")
      local new_sb_chunk = mod_statusbar.template_to_table("%start_insert_by_"..meter.." "..
							   chunk.." %end_insert_by_"..meter)
      local new_sb = old_sb
      table.remove(new_sb, pos)
      for i,v in pairs(new_sb_chunk) do
	 table.insert(new_sb, pos, v)
	 pos = pos + 1
      end
      this.StatusBar:set_template_table(new_sb)
   end

   -- restores the statusbar with the original meter
   function this.restore_sb(meter)
      local st, en
      local old_sb = this.StatusBar:get_template_table()
      for a, item in pairs(old_sb) do
	 if item.meter == "start_insert_by_"..meter then 
	    st = a
	 end
	 if item.meter == "end_insert_by_"..meter then 
	    en = a
	    break
	 end
      end
      local new_sb = old_sb
      if en then	
	 for a=st, en, 1 do 
	    table.remove(new_sb, st)
	 end
	 table.insert(new_sb, st, this.backup[meter])
	 this.StatusBar:set_template_table(new_sb)
	 mod_statusbar.inform(meter, "")
      end
   end

   -- gets ticket's data
   function this.get_record(t)
      local command = "wget -O "..this.paths.sessiondir.."/"..
	 string.gsub(t,"%^" ,"")..".cvs "..this.url.."?s="..
	 string.gsub(t,"%^" ,"%%5E")..
	 "\\&f=sl1d1t1c1ohgv\\&e=.csv"
      os.execute(command)
      local f = io.open(this.paths.sessiondir .."/"..string.gsub(t,"%^" ,"")..".cvs", "r")
      if not f then return end
      local s=f:read("*all")
      f:close()
      os.execute("rm "..this.paths.sessiondir .."/"..string.gsub(t,"%^" ,"")..".cvs")
      return s
   end

   -- parses ticket's data and store them in this.data.ticketname
   function this.process_record(s)
      local _,_,t = string.find(s, '"%^?(.-)".*' ) 
      t = string.gsub(t , "%.", "")
      this.data[t] = { raw_data = s, }
      _, _, 
      this.data[t].ticket,
      this.data[t].quote,
      this.data[t].date,
      this.data[t].time,
      this.data[t].difference,
      this.data[t].open,
      this.data[t].high,
      this.data[t].low,
      this.data[t].volume =
	 string.find(s, '"(.-)",(.-),"(.-)","(.-)",(.-),(.-),(.-),(.-),(.-)' )
      if tonumber(this.data[t].difference) ~= nil then
	 this.data[t].delta = math.dpr((this.data[t].difference /
					(this.data[t].quote - this.data[t].difference) * 100), 2)
      else
	 this.data[t] = nil
      end
   end

   -- updates tickets' data
   function this.update_data()
      for _, v in pairs(this.config.tickets) do
	 this.process_record(this.get_record(v))
      end
   end

   -- gets threshold info
   function this.get_hint(meter, val)
      local hint = "normal"
      local crit = this.config.critical[meter]
      local imp = this.config.important[meter]
      if crit and tonumber(val) < crit then
	 hint = "critical"
      elseif imp and tonumber(val) >= imp then
	 hint = "important"
      end
      return hint
   end

   -- gets the unit (if any) of each meter
   function this.get_unit(meter)
      local unit = this.config.unit[meter]
      if unit then return unit end
      return ""
   end

   -- gets the quantity (if any) of each ticket
   function this.get_quantity(t)
      local quantity = this.config.portfolio[t]
      if quantity then return quantity end
      return 1
   end

   -- notifies data to statusbar
   function this.notify()
      local newtmpl = ""
      local perf = 0
      local base = 0
      for i,v in pairs(this.data) do
	 newtmpl = newtmpl..v.ticket..": %stock_delta_"..i.." "
	 perf = perf + this.get_quantity(v.ticket) + (v.delta * this.get_quantity(v.ticket) / 100)
	 base = base + (this.get_quantity(v.ticket))
	 for ii,vv in pairs(this.data[i]) do
	    mod_statusbar.inform("stock_"..ii.."_"..i.."_hint", this.get_hint(ii, vv))
	    mod_statusbar.inform("stock_"..ii.."_"..i, vv..this.get_unit(ii))
	 end
      end
      if this.config.toggle == "off" then
	 if perf > base then
	    mod_statusbar.inform("stock",this.config.off_msg )
	    mod_statusbar.inform("stock_hint", "important")
	 else
	    mod_statusbar.inform("stock",this.config.off_msg )
	    mod_statusbar.inform("stock_hint", "critical")
	 end
      else
	 this.sb_insert_tmpl_chunk(newtmpl, "stock")
      end
      mod_statusbar.update()
   end

   -- checks if the timer is set and ther restarts
   function this.restart()
      if not this.status_timer then this.resume() end
      this.loop()
   end

   -- main loop
   function this.loop()
      if this.status_timer ~= nil and mod_statusbar ~= nil then
	 this.update_data()
	 this.notify()
	 this.status_timer:set(this.config.interval, this.loop)
      end
   end
   
   -- public methods

   function this.update()
      this.loop()
   end

   function this.add_ticket(mplex)
      local handler = function(mplex, str)
			 local _,_,t,_,q = string.find(str, "(.*)(,)(%d*)")
			 ioncore.write_savefile("debug", { ["q"] = q, ["t"] = t})
			 if q then this.config.portfolio[t] = tonumber(q)
			 else this.config.portfolio[str] = 1 end 
			 ioncore.write_savefile("cfg_stock", this.config.portfolio)
			 this.process_portfolio()
			 this.restart()
		      end
      mod_query.query(mplex, TR("Add a ticket (format: ticketname - e.g. ^N225 or tickename,quantity e.g: ^N225,100):"),
		      nil, handler, nil, "stock")
   end

   function this.del_ticket(mplex)
      local handler = function(mplex, str)
			 for i,v in pairs(this.config.tickets) do
			    if this.config.tickets[i] == str then 
			       this.config.tickets[i] = nil
			       break
			    end
			 end
			 this.config.portfolio[str] = nil
			 ioncore.write_savefile("cfg_stock", this.config.portfolio)
			 this.data[string.gsub(str,"[%^%.]" ,"")] = nil
			 this.restart()
		      end
      mod_query.query(mplex, TR("Delete a ticket (format: tickename e.g. ^N225):"), nil, handler, 
		      nil, "stock")
   end

   function this.suspend()
      this.restore_sb("stock")
      mod_statusbar.inform("stock",this.config.susp_msg )
      mod_statusbar.inform("stock_hint", this.config.susp_msg_hint)
      mod_statusbar.update()
      this.status_timer = nil
   end

   function this.resume()
      this.status_timer = ioncore.create_timer()
      this.restart()
   end

   function this.toggle()
      if this.config.toggle == "on" then
	 this.config.toggle = "off"
	 this.restore_sb("stock")
	 mod_statusbar.inform("stock",this.config.off_msg )
      else
	 this.config.toggle = "on"
      end
      this.restart()
   end
   
   -- constructor
   function this.init()
      this.process_config()
      this.loop()
   end
   
   this.init()
   -- return this
   return { 
      update = this.update,
      add_ticket = this.add_ticket,
      del_ticket = this.del_ticket,
      toggle = this.toggle,
      suspend = this.suspend,
      resume = this.resume,
      data = this.data,
      config = this.config,
   }
end

-- there we go!
StockMonitor = new_stock()
