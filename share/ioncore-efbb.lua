--
-- ion/share/ioncore-efbb.lua -- Minimal emergency fallback bindings.
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

warn([[
Some binding groups were empty. Making the following minimal emergency
mappings:
  F2 -> xterm
  F11 -> restart
  F12 -> exit
  Mod1+C -> close
  Mod1+K P/N -> WFrame.switch_next/switch_prev
]])


defbindings("WScreen", {
    kpress("F2", function() ioncore.exec('xterm')  end),
    kpress("F11", function() ioncore.restart() end),
    kpress("F12", function() ioncore.exit() end),
})

defbindings("WMPlex", {
    kpress_waitrel("Mod1+C", WRegion.rqclose_propagate),
})

defbindings("WFrame", {
    submap("Mod1+K", {
        kpress("AnyModifier+N", function(f) f:switch_next() end),
        kpress("AnyModifier+P", function(f) f:switch_prev() end),
    })
})
