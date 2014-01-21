-- Authors: Unknown
-- License: Unknown
-- Last Changed: Unknown
-- 
-- Move current window in a frame to another frame in specified direction

move_current={}

function move_current.move(ws, dir)
    local frame=ws:current()
    local cwin=frame:current()
    local frame2=ioncore.navi_next(frame,dir)
    
    if frame2 then
        frame2:attach(cwin, { switchto=true })
    end
    cwin:goto()
end

defbindings("WTiling", {
    submap("Mod1+K", {
        kpress("Up", function(ws) move_current.move(ws, "up") end),
        kpress("Down", function(ws) move_current.move(ws, "down") end),
        kpress("Left", function(ws) move_current.move(ws, "left") end),
        kpress("Right", function(ws) move_current.move(ws, "right") end),
    }),
})
