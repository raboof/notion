-- Authors: Canaan Hadley-Voth
-- License: Unknown
-- Last Changed: Unknown
--
-- bindsearch.lua
-- 
-- Canaan Hadley-Voth.
--
-- Bindings are spread across distant files, some are created
-- dynamically, and some are negated by others.  I use this to 
-- tell me exactly what is bound right now.
--
-- Search (in all sections) for bound keys or bound commands.
-- There can be multiple search terms separated by spaces, but
-- they would have to appear in the specified order.  Output 
-- is a WMessage with a section of the bindings table.
-- 
--[[

Example: bindsearch("K X") yields:

WIonWS
 [kcb] = Mod1+K
 [action] = kpress
 [submap] = table: 0x8254478
 [cmd] = WIonWS.unsplit_at(_, _sub)
 [action] = kpress
 [kcb] = AnyModifier+X
 [func] = function: 0x80de2d0

cmdsearch("unsplit") would have found the same thing.

A search for " " shows all bindings.

--]]


-- In versions older than 20051210, print() is not defined to send wmessages.
-- Uncommenting this next line might be necessary.
-- local function print(msg) mod_query.message( ioncore.region_list("WScreen")[1], tostring(msg) ) end

local function strtable(tabl, f)
    local msg=""
    if not f then f=function(x) return x end end
    n=#tabl
    if n>0 then
	msg=tostring(n).." objects:"
    end
    for i in pairs(tabl) do
        msg=msg.." ["..tostring(i).."] = "..tostring(f(tabl[i])).."\n"
    end
    return msg
end


-- Possible fields are "kcb" (default), "cmd", and "class".
function bindsearch(str, field)

    local btabl = table.copy(ioncore.getbindings(), true)
    local fullmsg=""
    local fullkcb=""
    local class=""
    str=string.gsub(str, "%+", "%%%+")
    str=string.gsub(str, " ", ".*")
    --string.gsub(str, "@", "%%%@")

    local function domap(class, t, msg, kcb, show_all)
	local found
	for index,bind in pairs(t) do
	    found=false
	    if bind.area then
		fullkcb=kcb.." "..bind.kcb.."@"..bind.area
	    else
		fullkcb=kcb.." "..bind.kcb
	    end
	    fullkcb=string.gsub(fullkcb, "AnyModifier", " ")
	    if (field==nil or field=="kcb") and string.find(fullkcb, str) then
		found=true
	    elseif field=="cmd" and bind.cmd and 
	      string.find(string.lower(bind.cmd), string.lower(str)) then
		found=true
	    elseif field=="class" and class==str then
		found=true
	    end
	    if bind.submap then
		domap(class, bind.submap, msg..strtable(bind), fullkcb, found)
	    elseif found or show_all then
		fullmsg=fullmsg..msg..strtable(bind)
	    end
	end
    end

    for class, bindings in pairs(btabl) do
	domap(class, bindings, class.."\n", "", false)
    end
    print(fullmsg)
end

function cmdsearch(str)
    bindsearch(str, "cmd")
end
