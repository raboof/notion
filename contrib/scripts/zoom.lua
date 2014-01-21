-- Authors: Rene van Bevern <rvb@pro-linux.de>, Etan Reisner <deryni@gmail.com>
-- License: Public domain
-- Last Changed: Unknown
-- 
-- zoom.lua
--
-- exchanges current client window with the active client window of the frame
-- called 'zoomframe' -- resembling the behavior of larswm
--
-- +---------------------------------------------------+-----------------+
-- |                                                   |                 |
-- |                                                   |                 |
-- |                                                   |                 |
-- |                                                   |   Client 1      |
-- |                                                   |                 |
-- |                                                   |                 |
-- |            Z o o m f r a m e  w i t h             +-----------------+
-- |            z o o m e d   C l i e nt               |                 |
-- |                                                   |                 |
-- |                                                   |   Client 2      |
-- |                                                   |                 |
-- |                                                   |                 |
-- |                                                   |                 |
-- +--------------------------------+------------------+-----------------+
-- |                                |                                    |
-- |                                |                                    |
-- |        Client 4                |            Client 3                |
-- |                                |                                    |
-- |                                |                                    |
-- +--------------------------------+------------------------------------+
--
-- Example: zoom_client on "Client 2" will put "Client 2" into the zoom frame
-- and "zoomed Client" into the frame of "Client 2"
--
-- By Rene van Bevern <rvb@pro-linux.de>, 2005
-- Modifications by Etan Reisner <deryni in #ion>
-- Public Domain
--
-- Example keybindings:
--
-- defbindings("WFrame", {
--    kpress(META.."z", "zoom_client(_, _sub)")
--    kpress(META.."z", "zoom_client(_, _sub, {swap=true})")
--    kpress(META.."z", "zoom_client(_, _sub, {swap=false, goto=true})")
-- }
--
-- zoom_client accepts a table that may contain the options 'swap'
-- and 'goto'. Both are by default set to 'true'.
--
-- 'goto' controls whether to switch to the zoomframe
--
-- 'swap' controls whether to bring the zoomframe's active client
--    into the current frame
--
--
-- Example configuration setup:
--
-- zoom_client_set({zoomframename = 'foo'})

local settings = {
   zoomframename = 'zoomframe'
}

function zoom_client_set(tab)
   settings = table.join(tab, settings)
end

function zoom_client(curframe, curclient, options)
   local zoomframe = ioncore.lookup_region(settings.zoomframename, 'WFrame')
   if options == nil then
   	options = {}
   end
   if options.goto == nil then
      options.goto = true
   end
   if options.swap == nil then
      options.swap = true
   end
   if (not zoomframe) or (curframe == zoomframe) then
      return
   end
   local zoomclient = zoomframe:mx_current()
   if curclient then
      zoomframe:attach(curclient)
   end
   if zoomclient and options.swap then
      curframe:attach(zoomclient)
   end
   if zoomclient and options.goto then
      zoomclient:goto() -- make it activated in the frame
   end
   if curclient and options.goto then
      curclient:goto()
   end
end
