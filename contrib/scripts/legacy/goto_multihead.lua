-- Authors: Unknown
-- License: Unknown
-- Last Changed: Unknown
-- 
-- A version of ioncore.goto_next that may be useful on multihead setups
-- Replace ioncore.goto_next with goto_multihead.goto_next in cfg_tiling.lua
-- to use it.

goto_multihead={}

function goto_multihead.goto_next(ws, dir)
    if dir=="up" or dir=="down" then
        ioncore.goto_next(ws:current(), dir)
        return
    end
    
    local nxt, nxtscr
    
    nxt=ioncore.navi_next(ws:current(), dir, {nowrap=true})
    
    if not nxt then
        local otherdir
        local fid=ioncore.find_screen_id
        if dir=="right" then
            otherdir="left"
            nxtscr=fid(ws:screen_of():id()+1) or fid(0)
        else
            otherdir="right"
            nxtscr=fid(ws:screen_of():id()-1) or fid(-1)
        end
        nxt=nxtscr:current():current()
        if obj_is(nxt, "WTiling") then
            nxt=nxt:farthest(otherdir)
        end
    end
    
    nxt:goto_focus()
end
