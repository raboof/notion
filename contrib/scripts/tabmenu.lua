--
-- tabmenu.lua
--
-- By Tuomo Valkonen, 2007.
--
-- This script introduces a tabmenu function that can be used to
-- display a grabmenu corresponding to the tabs of a frame. It can
-- be useful if you choose to disable the tab-bars of frames, to 
-- replace the META+K N/P bindings with the following:
--
-- ioncore.defbindings("WFrame.toplevel", {
--    submap(META.."K", {
--        kpress("N", "tabmenu.tabmenu(_, _sub, 'next')"),
--        kpress("P", "tabmenu.tabmenu(_, _sub, 'prev')"),
--    })
-- })
--
--

tabmenu={}

-- WRegion.displayname isn't exported ATM, so here's a hacky
-- implementation.
function tabmenu.hack_displayname(reg)
    local name=reg:name()
    if obj_is(reg, "WGroupCW") then
        local b=reg:bottom()
        if b then
            name=b:name()
        end
    end
    return name
end


function tabmenu.tabmenu(_, _sub, index)
    local i=1
    local m={}
    _:mx_i(function(r)
               local e=menuentry(tabmenu.hack_displayname(r),
                                 function() r:goto() end)
               table.insert(m, e)
                if r==_sub
                    then i=#m
                end
                return true
            end)
    
    if #m==0 then
        table.insert(m, menuentry("<empty frame>", function() end))
        i=nil
    elseif index=='next' then
        i=((i==#m and 1) or i+1)
    elseif index=='prev' then
        i=((i==1 and #m) or i-1)
    elseif index=='first' then
        i=1
    elseif index=='last' then
        i=#m
    end
    
    mod_menu.grabmenu(_, nil, m, {initial=i})
end

