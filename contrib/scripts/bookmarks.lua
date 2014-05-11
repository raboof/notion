-- Authors: Unknown
-- License: Unknown
-- Last Changed: Unknown
-- 
-- Bookmarks support for Ion3
-- 
-- MOD1+b n        Go to bookmark n (n=0..9)
-- MOD1+b Shift+n  Set bookmark n
-- 

local bms={}
bookmarks={}

function bookmarks.set(bm, frame)
    bms[bm]=frame
end

function bookmarks.goto_bm(bm)
    if bms[bm] then
        bms[bm]:goto_focus()
    end
end


for k=0, 9 do
    local bm=tostring(k)
    defbindings("WScreen", {
        submap(MOD1.."b", {
            kpress(bm, function() bookmarks.goto_bm(bm) end),
        })
    })
    defbindings("WFrame", {
        submap(MOD1.."b", {
            kpress("Shift+"..bm, 
                   function(frame) bookmarks.set(bm, frame) end),
        })
    })
end

