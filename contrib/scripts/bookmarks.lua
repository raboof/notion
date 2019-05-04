-- Authors: Unknown
-- License: Unknown
-- Last Changed: Unknown
-- 
-- Bookmarks support for Ion3
-- 
-- META+b n        Go to bookmark n (n=0..9)
-- META+b Shift+n  Set bookmark n
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
        submap(META.."b", {
            kpress(bm, function() bookmarks.goto_bm(bm) end),
        })
    })
    defbindings("WFrame", {
        submap(META.."b", {
            kpress("Shift+"..bm, 
                   function(frame) bookmarks.set(bm, frame) end),
        })
    })
end

