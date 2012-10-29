-- Authors: Canaan Hadley-Voth <ugggg@hotmail.com>
-- License: Unknown
-- Last Changed: Unknown
-- 
-- send_to_ws.lua
--
-- written by Canaan Hadley-Voth, ugggg at hotmail
--
-- Sends a clientwin to another workspace.
--
-- On a tiled workspace, the frame sent to will be the most recently active.  
-- Maximized windows are skipped over in search of an actual workspace.
-- Focus follows if jumpto is set.
--
-- send_to_ws(cwin, action, jumpto)
-- send_to_new_ws(cwin, wstype, jumpto)
--
-- The action argument can be a specific workspace number OR it can be
-- 'left', 'right', or 'any'.
--
-- Workspace numbers start at 0 because WScreen.switch_nth starts at 0.
-- (In both cases the bindings make it look like they start at 1.)
--
--

-- These are the bindings I use, as an example.
defbindings("WMPlex", {
    submap(META.."K", {
        submap("bracketleft", {
	    kpress("H", "send_to_ws(_sub, 'left', true)"),
	    kpress("L", "send_to_ws(_sub, 'right', true)"),
	    kpress("1", "send_to_ws(_sub, 0, true)"),
	    kpress("2", "send_to_ws(_sub, 1, true)"),
	    kpress("3", "send_to_ws(_sub, 2, true)"),
	    kpress("4", "send_to_ws(_sub, 3, true)"),
	    kpress("5", "send_to_ws(_sub, 4, true)"),
	    kpress("6", "send_to_ws(_sub, 5, true)"),
	    kpress("7", "send_to_ws(_sub, 6, true)"),
	    kpress("8", "send_to_ws(_sub, 7, true)"),
	    kpress("9", "send_to_ws(_sub, 8, true)"),
	    kpress("F", "send_to_new_ws(_sub, 'WGroupWS')"),
        }),
    }),
})


function send_to_ws(cwin, action, jumpto)
    local destws, destwsindex, destwstype
    local offset=0
    local scr=cwin:screen_of()
    local curws=cwin:manager()
    while curws:manager()~=scr do
	curws=curws:manager()
    end
    local curwsindex=scr:get_index(curws)
    
    if type(action)=="string" then
	if action=="left" then
	    offset=-1
	elseif action=="right" then
	    offset=1
	elseif action=="any" then
	    -- send to the right if a workspace exists there, otherwise left
	    if not(WMPlex.mx_nth(scr, curwsindex+1)) then
		send_to_ws(cwin, 'left', jumpto)
		return
	    else
		send_to_ws(cwin, 'right', jumpto)
		return
	    end
	end
	
	-- Skip over fullscreen client windows or frames or whatever else,
	-- until an actual workspace is found.
	destwsindex=curwsindex
	destws=scr:mx_nth(destwsindex+offset)
	destwstype=obj_typename(destws)
	while destwstype~="WGroupWS" and destwstype~="WTiling" do
	    destwsindex=destwsindex+offset
	    destws=scr:mx_nth(destwsindex)
	    destwstype=obj_typename(destws)
	    -- If there are no workspaces to the right, make one.
	    --	 (workspace type created can be set to "WGroupWS" instead.
	    -- If there are no workspaces to the left, give up.
	    if destwsindex>scr:mx_count() then
		send_to_new_ws(cwin, "WTiling")
		return
	    elseif destwsindex<0 then
		return
	    end
	end
    elseif type(action)=="number" then
	destwsindex=action
	destws=scr:mx_nth(destwsindex)
	destwstype=obj_typename(destws)
	if destwsindex==curwsindex then return end
    end

    if destwstype=="WTiling" then
	destws:current():attach(cwin, {switchto=true})
    elseif destwstype=="WGroupWS" and obj_typename(destws:current())=="WTiling" then
	destws:current():current():attach(cwin, {switchto=true})
    elseif destwstype=="WGroupWS" then
	destws:attach_framed(cwin)
    end

    if jumpto then
	cwin:goto()
    end
end


function send_to_new_ws(cwin, wstype, jumpto)
    local scr=cwin:screen_of()
    local n=scr:mx_count()
    local newws

    if wstype=="WGroupWS" then
	newws=scr:attach_new{type="WGroupWS"}
	newws:attach_framed(cwin)
    else
	newws=scr:attach_new{type="WTiling"}
	newws:current():attach(cwin)
    end
    if jumpto then cwin:goto() end
end
