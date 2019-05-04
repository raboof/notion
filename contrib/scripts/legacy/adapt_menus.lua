-- Authors: David Tweed
-- License: Unknown
-- Last Changed: September 2005
--
-- this builds up a menu framework for performing some client operation
-- say, jumping to the client or attaching the client ot the current 
-- frame. it assumes that helpful applications either tend to have titles of
-- the form or will allow their title s to be set to something roughly of the
-- form "<type>: <details>", so the top-level menu performs a split on the
--type, then the first level
-- of submenus contains the details portion of the titles. (title entries
-- that don't fit the "type:" pattern get stuffed into an submenu labelled "misc.".)

-- however, if
-- a first level menu would exceed a certain size then a grouping into
-- another layer of submenus is performed on the basis of the first 
-- letter of detail string (suppressing sub-menus that would be below a
-- certain number of elements). note the aim is NOT consistency across
-- invocations but to try and on-the-fly make clients as easy to find & access
-- as possible (in an some overall sense). the acutal algorithm is a bit ad-hoc.

-- this was motivated by usage of Ion3 where every frame had it's tab-bar
-- hidden (partly because with LOTS of windows the tabs become
-- so small that trying to read them doesn't work too well) and has
-- become the primary navigation mechanism. it'll probably be of less
-- interest when there aren't enough windows to make tabs ineffective.

-- the four useful client-functions -- closing, jumping-to, tagging and attaching-here --
-- are provided. anyone coming up with other useful functions is encouraged to
-- add them to the version of this file on the ion3-scripts archive
-- the commented out keybindings at the end are just to show how the
-- functionality might be used.

-- tested to work on ion-3ds-20050820.

-- Changelog
-- Sep 2005, initial version, David Tweed

function mkentryJump(frame,tgt,name)
    return ioncore.menuentry(name, function() tgt:goto_focus() end)
end

function mkentryClose(frame,tgt,name)
    return ioncore.menuentry(name, function() tgt:rqclose() end)
end

function mkentryTag(frame,tgt,name)
    return ioncore.menuentry(name, function() tgt:set_tagged("toggle") end)
end

function attachHere(frame,tgt)
    if tgt:manager()==frame then
        tgt:goto_focus()
    else
        frame:attach(tgt,{switchto=true})
    end
end

function mkentryAttach(frame,tgt,name)
    return ioncore.menuentry(name, function() attachHere(frame,tgt) end)
end

function sizeSubmenus(tbl)
         local noMenus=0
         local totSz=0
         for i,w in pairs(tbl) do
             totSz=totSz+#(w)
             noMenus=noMenus+1
         end
         return totSz,noMenus
end

--treat two tables as lists and concatenate them (ie ignore keys)
function extend(lst1,lst2)
    local lst=lst1
    for _,v in pairs(lst2) do
         table.insert(lst,v)
    end
    return lst
end

-- for fonts where the width of two spaces is the same as the
-- width of any digit this alignment hack will work
function prependNum(dict)
    table.sort(dict, function(a, b) return a.name < b.name end)
    local cnt=0
    for i,w in pairs(dict) do
        local aligner
        if cnt<10 then
            aligner="  "
        else
            aligner=""
        end
        w.name=string.format("%u.%s %s",cnt,aligner,w.name)
        cnt=cnt+1
    end
end

function collapseTwoLevels(menu,mkentry,dict,frame,attr)
    for j,w in pairs(dict) do
        for k,w1 in pairs(w) do
            table.insert(menu,mkentry(frame,w1.obj,w1[attr]))
        end
    end
end

function insertListIntoMenu(dest,mkentry,dict,frame,strKey)
    for i,v in pairs(dict) do
        table.insert(dest,mkentry(frame,v.obj,v[strKey]))
    end
end

function makelistEngine(frame,title,mkentry,lst,
            maxSzForMainMenuMerge)
    local subEntryDB={}
    for i,tgt in pairs(lst) do
        local n=tgt:name()
        local c="misc."
        local rest=n
        local s,f=string.find(n,"^([^ :]+[:])")
        if s then
            c=string.sub(n,s,f)
            rest=string.sub(n,f+1)
       else
            rest=n
       end
       local r1=string.sub(rest,1,1)
       if not(subEntryDB[c]) then
            subEntryDB[c]={}
       end
       if not(subEntryDB[c][r1]) then
            subEntryDB[c][r1]={}
       end
       -- store enough so that we can apply mkentry once we know where in the
       -- hierarchy this should appear (and thus what it's text string should be)
       table.insert(subEntryDB[c][r1],{title=n,rest=rest,obj=tgt})
    end
    local mainMenu={}
    for i,v in pairs(subEntryDB) do
        local sz,noSubMenus=sizeSubmenus(v)
        local sndLevMenu={}
        --if we'd have a group of submenus of total size less than maxSzForMainMenuMerge,
        --put them directly onto the main menu without submenus
        if sz<=maxSzForMainMenuMerge then
            -- use the full title on "menu direct entires"
           collapseTwoLevels(mainMenu,mkentry,v,frame,"title")
        else
            if sz<=5 or noSubMenus<=2 then
                -- use sub-instance title for "sub menu all entries direct entries"
                collapseTwoLevels(sndLevMenu,mkentry,v,frame,"rest")
            else
                for j,w in pairs(v) do
                    --if we'd only have 1 or 2 element submenu, stick them in submenu directly 
                    if #(w)<=2 then
                            -- use the sub-instance on "sub-menu direct entries"
                            insertListIntoMenu(sndLevMenu,mkentry,w,frame,"rest")
                    else
                       local thdLevMenu={}
                       -- use the sub-instance on "sub-sub-menu entries"
                       insertListIntoMenu(thdLevMenu,mkentry,w,frame,"rest")
                       prependNum(thdLevMenu)
                       table.insert(sndLevMenu,ioncore.submenu(j.."...",thdLevMenu,1))
                   end
                end
            end
            prependNum(sndLevMenu)
            table.insert(mainMenu,ioncore.submenu(i,sndLevMenu,1))
        end
    end
    prependNum(mainMenu)
    table.insert(mainMenu,#(mainMenu)+1,ioncore.menuentry("------------------"..title.."------------------", "nil"))
    return mainMenu
end

--[[
-- some examples of usage
defbindings("WFrame", {
    kpress(META.."F8", "mod_menu.menu(_,_sub,makelistEngine(_,'jump',mkentryJump,extend(ioncore.clientwin_list(),ioncore.region_list('WGenWS')),1))"),
    kpress(MOD2.."F8", "mod_menu.menu(_,_sub,makelistEngine(_,'jumpWS',mkentryJump,ioncore.region_list('WGenWS'),4))"),
    kpress(META.."F7", "mod_menu.menu(_, _sub,makelistEngine(_,'attach',mkentryAttach,ioncore.clientwin_list(),1))"),
    kpress(MOD2.."F7", "mod_menu.menu(_, _sub,makelistEngine(_,'close',mkentryClose,ioncore.clientwin_list(),1))"),
    kpress(META.."F6", "mod_menu.menu(_, _sub,makelistEngine(_,'raise',mkentryJump,extend(_:llist(1),ioncore.region_list('WGenWS')),4))"),
    kpress(MOD2.."F6", "mod_menu.menu(_, _sub,makelistEngine(_,'close',mkentryClose,_:llist(1),4))"),
})
--]]
