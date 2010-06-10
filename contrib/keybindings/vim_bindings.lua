-- vim-bindings.lua
--
-- This only affects queries.
-- 
-- Features:
--	Vi-keys for normal and insert modes.
--	Vim-style highlighting.  
--	Numbered prefixes work for most maneuvers.
-- 
-- Limitations:
--	All WEdln objects share the same mode and undo history.
--	No r replace
--	No . repeat last action
--	No character search.
--	3cw will work; c3w won't.
--	Not all unused symbols are silenced in normal mode
--	    (so don't type them)
--	Use Shift+Insert to paste from other apps, p to paste yanked text.
--
vim={clip="",old="",oldpoint=0}
function vim.normal_mode()
    defbindings("WEdln", {
	kpress("a", "{WEdln.forward(_), vim.insert_mode(), vim.savehist(_)}"),
	kpress("Shift+a", "{WEdln.eol(_), vim.insert_mode(), vim.savehist(_)}"),
	kpress("b", "vim.multiply(WEdln.bskip_word, _)"),
	kpress("Shift+b", "vim.multiply(WEdln.bskip_word, _)"),
	submap("c", {
	    kpress("b", "vim.yank(_, 'b', true, true)"),
	    kpress("c", "vim.yank(_, 'd', true, true)"),
	    kpress("h", "vim.yank(_, 'h', true, true)"),
	    kpress("l", "vim.yank(_, 'l', true, true)"),
	    kpress("w", "vim.yank(_, 'w', true, true)"),
	    kpress("0", "vim.yank(_, '0', true, true)"),
	    kpress("Shift+6", "vim.yank(_, '0', true, true)"),
	    kpress("Shift+4", "vim.yank(_, 'D', true, true)"),
	}),
	kpress("Shift+c", "vim.yank(_, 'D', true, true)"),
	submap("d", {
	    kpress("b", "vim.yank(_, 'b', true)"),
	    kpress("d", "vim.yank(_, 'd', true)"),
	    kpress("h", "vim.yank(_, 'h', true)"),
	    kpress("l", "vim.yank(_, 'l', true)"),
	    kpress("w", "vim.yank(_, 'w', true)"),
	    kpress("0", "vim.yank(_, '0', true)"),
	    kpress("Shift+6", "vim.yank(_, '0', true)"),
	    kpress("Shift+4", "vim.yank(_, 'D', true)"),
	}),
	kpress("Shift+d", "vim.yank(_, 'D', true)"),
	kpress("e", "{_:forward(), vim.multiply(WEdln.skip_word,_), _:back()}"),
	kpress("Shift+e", "{_:forward(), vim.multiply(WEdln.skip_word,_), _:back()}"),
	kpress("f", "vim.cleardigit()"),
	kpress("g", "vim.cleardigit()"),
	kpress("h", "vim.multiply(WEdln.back,_)"),
	kpress("i", "{vim.insert_mode(), vim.savehist(_)}"),
	kpress("Shift+i", "{WEdln.bol(_), vim.insert_mode(), vim.savehist(_)}"),
	kpress("j", "vim.multiply(WEdln.history_next, _)"),
	kpress("k", "vim.multiply(WEdln.history_prev, _)"),
	kpress("Control+n", "vim.multiply(WEdln.history_next, _)"),
	kpress("Control+p", "vim.multiply(WEdln.history_prev, _)"),
	kpress("l", "vim.multiply(WEdln.forward, _)"),
	kpress("m", "vim.cleardigit()"),
	kpress("n", "vim.cleardigit()"),
	kpress("o", "vim.cleardigit()"),
	kpress("p", "{WEdln.forward(_), vim.multiply(vim.paste,_,true),"
	    .."WEdln.back(_)}"),
	kpress("Shift+p", "vim.multiply(vim.paste,_,true)"),
	kpress("q", "vim.cleardigit()"),
	kpress("r", "vim.cleardigit()"),
	kpress("s", "vim.yank(_, 'l', true, true)"),
	kpress("Shift+s", "vim.yank(_, 'd', true, true)"),
	kpress("t", "vim.cleardigit()"),
	kpress("u", "vim.undo(_)"),
	kpress("v", "vim.highlight(_)"),
	kpress("w", "vim.multiply(WEdln.skip_word, _)"),
	kpress("Shift+w", "vim.multiply(WEdln.skip_word, _)"),
	kpress("x", "vim.x(_)"),
	kpress("Shift+x", "vim.yank(_, 'h', true)"),
	kpress("y", "vim.yank(_)"),
	kpress("Shift+y", "vim.yank(_, 'd')"),
	kpress("z", "vim.cleardigit()"),
	kpress("0", "vim.zero(_)"),
	kpress("1", "vim.dodigit(1)"),
	kpress("2", "vim.dodigit(2)"),
	kpress("3", "vim.dodigit(3)"),
	kpress("4", "vim.dodigit(4)"),
	kpress("5", "vim.dodigit(5)"),
	kpress("6", "vim.dodigit(6)"),
	kpress("7", "vim.dodigit(7)"),
	kpress("8", "vim.dodigit(8)"),
	kpress("9", "vim.dodigit(9)"),
	kpress("Shift+6", "{vim.cleardigit(), WEdln.bol(_)}"),
	kpress("Shift+4", "{vim.cleardigit(), WEdln.eol(_)}"),
	kpress("BackSpace", "vim.multiply(WEdln.back, _)"),
	kpress("space", "vim.multiply(WEdln.forward, _)"),
	kpress("Shift+grave", "vim.multiply(vim.switchcase,_,true)"),
	kpress("Control+A", "vim.multiply(vim.increment, _)"),
	kpress("Control+X", "vim.multiply(vim.decrement, _)"),

	kpress("Shift+f", "vim.cleardigit()"),
	kpress("Shift+g", "vim.cleardigit()"), kpress("Shift+h", "vim.cleardigit()"),
	kpress("Shift+j", "vim.cleardigit()"), kpress("Shift+k", "vim.cleardigit()"), 
	kpress("Shift+l", "vim.cleardigit()"), kpress("Shift+m", "vim.cleardigit()"),
	kpress("Shift+n", "vim.cleardigit()"), kpress("Shift+o", "vim.cleardigit()"),
	kpress("Shift+q", "vim.cleardigit()"), kpress("Shift+r", "vim.cleardigit()"),
	kpress("Shift+t", "vim.cleardigit()"), kpress("Shift+u", "vim.cleardigit()"),
	kpress("Shift+v", "vim.cleardigit()"),
	kpress("Shift+z", "vim.cleardigit()"), kpress("Shift+0", "vim.cleardigit()"),
	kpress("Shift+1", "vim.cleardigit()"), kpress("Shift+2", "vim.cleardigit()"), 
	kpress("Shift+3", "vim.cleardigit()"), kpress("Shift+5", "vim.cleardigit()"),
	kpress("Shift+7", "vim.cleardigit()"), kpress("Shift+8", "vim.cleardigit()"), 
	kpress("Shift+9", "vim.cleardigit()"),
    })
end

function vim.insert_mode()
    for _,edln in pairs(ioncore.region_list("WEdln")) do
	edln:clear_mark()
    end
    defbindings("WEdln", {
	kpress("Control+w", "WEdln.bkill_word(_)"),
	kpress("Control+n", "WEdln.next_completion(_, true)"),
	kpress("Control+p", "WEdln.prev_completion(_, true)"),
	kpress("Escape", "vim.normal_mode()"),
	kpress("Control+bracketleft", "vim.normal_mode()"),

	kpress("BackSpace", "WEdln.backspace(_)"),
	kpress("Mod1+BackSpace", "WEdln.bkill_word(_)"),
	kpress("a", nil), kpress("b", nil), kpress("c", nil),
	kpress("d", nil), kpress("e", nil), kpress("f", nil),
	kpress("g", nil), kpress("h", nil), kpress("i", nil),
	kpress("j", nil), kpress("k", nil), kpress("l", nil),
	kpress("m", nil), kpress("n", nil), kpress("o", nil),
	kpress("p", nil), kpress("q", nil), kpress("r", nil),
	kpress("s", nil), kpress("t", nil), kpress("u", nil),
	kpress("v", nil), kpress("w", nil), kpress("x", nil),
	kpress("y", nil), kpress("z", nil), kpress("0", nil),
	kpress("1", nil), kpress("2", nil), kpress("3", nil),
	kpress("4", nil), kpress("5", nil), kpress("6", nil),
	kpress("7", nil), kpress("8", nil), kpress("9", nil),
	kpress("Shift+a", nil), kpress("Shift+b", nil), kpress("Shift+c", nil),
	kpress("Shift+d", nil), kpress("Shift+e", nil), kpress("Shift+f", nil),
	kpress("Shift+g", nil), kpress("Shift+h", nil), kpress("Shift+i", nil),
	kpress("Shift+j", nil), kpress("Shift+k", nil), kpress("Shift+l", nil),
	kpress("Shift+m", nil), kpress("Shift+n", nil), kpress("Shift+o", nil),
	kpress("Shift+p", nil), kpress("Shift+q", nil), kpress("Shift+r", nil),
	kpress("Shift+s", nil), kpress("Shift+t", nil), kpress("Shift+u", nil),
	kpress("Shift+v", nil), kpress("Shift+w", nil), kpress("Shift+x", nil),
	kpress("Shift+y", nil), kpress("Shift+z", nil), kpress("Shift+0", nil),
	kpress("Shift+1", nil), kpress("Shift+2", nil), kpress("Shift+3", nil),
	kpress("Shift+4", nil), kpress("Shift+5", nil), kpress("Shift+6", nil),
	kpress("Shift+7", nil), kpress("Shift+8", nil), kpress("Shift+9", nil),
	kpress("space", nil),
	kpress("Shift+grave", nil),
	kpress("Control+A", nil),
	kpress("Control+X", nil),
    })
end

defbindings("WInput", {
    kpress("Control+C", "{vim.cleardigit(), WInput.cancel(_)}"),
    kpress("Control+F", "WInput.scrollup(_)"),
    kpress("Control+B", "WInput.scrolldown(_)"),
    kpress("Page_Up", "WInput.scrollup(_)"),
    kpress("Page_Down", "WInput.scrolldown(_)"),
})
defbindings("WEdln", {
    kpress("Shift+Insert", "WEdln.paste(_)"),
    kpress("Return", "{vim.cleardigit(), WEdln.finish(_)}"),
})

function vim.yank(edln, how, kill, insert)
    vim.old=edln:contents()
    vim.oldpoint=edln:point()
    local first,last
    local n=1
    if vim.digit1 then n=vim.digit1 end
    if vim.digit10 then n=n+10*vim.digit10 end

    if edln:mark() < 0 then
	edln:set_mark()
	for i=1,n do
	    if how=="b" then
		edln:bskip_word()
	    elseif how=="D" then
		edln:eol()
	    elseif how=="d" then
		edln:bol()
		edln:set_mark()
		edln:eol()
	    elseif how=="h" then
		edln:back()
	    elseif how=="l" then
		edln:forward()
	    elseif how=="w" then
		edln:skip_word()
	    elseif how=="0" then
		edln:bol()
	    end
	end
    end
    first=edln:mark()+1
    last=edln:point()
    if first>last then
	first=last+1
	last=edln:mark()
    end
    vim.clip=string.sub(edln:contents(), first, last)
    if kill then
	edln:cut()
    else
	edln:copy()
    end
    edln:clear_mark() -- just in case?
    vim.cleardigit()
    if insert then
	vim.insert_mode()
    end
end

function vim.undo(edln)
    local str=edln:contents()
    edln:kill_line()
    edln:insstr(vim.old)
    vim.old=str
    edln:bol()
    for i=1,vim.oldpoint do
	edln:forward()
    end
end

function vim.savehist(edln)
    vim.old=edln:contents()
    vim.oldpoint=edln:point()
end

function vim.zero(edln)
    if vim.digit1 then
	vim.dodigit(0)
    else
	edln:bol()
	vim.cleardigit()
    end
end

function vim.x(edln)
    -- x is only a backspace when there is nothing ahead.
    if string.len(edln:contents()) == edln:point() then
	vim.yank(edln, 'h', true)
    else
	vim.yank(edln, 'l', true)
    end
end

function vim.highlight(edln)
    if edln:mark() > -1 then
	edln:clear_mark()
    else
	edln:set_mark()
    end
    vim.cleardigit()
end

function vim.paste(edln)
    -- if something is highlighted, replace it.
    local clip2=vim.clip
    if edln:mark() > -1 then
	first=edln:mark()+1
	last=edln:point()
	if first>last then
	    first=last+1
	    last=edln:mark()
	end
	vim.clip=string.sub(edln:contents(), first, last)
	edln:cut()
    end
    edln:insstr(clip2)
    vim.cleardigit()
end

function vim.multiply(f, edln, addtohistory)
    if edln and addtohistory then
	vim.old=edln:contents()
	vim.oldpoint=edln:point()
    end
    local n=1
    if vim.digit1 then n=vim.digit1 end
    if vim.digit10 then n=n+10*vim.digit10 end
    for i=1,n do
	f(edln)
    end
    vim.cleardigit()
end

function vim.dodigit(digit)
    vim.digit10=vim.digit1
    vim.digit1=digit
end

function vim.cleardigit()
    -- This function needs to go almost everywhere. Doing the wrong thing 
    -- 10 times because a number was pressed ages ago, would be bad.
    vim.digit1=nil
    vim.digit10=nil
end

function vim.switchcase(edln)
    local c=string.sub(edln:contents(), edln:point()+1, edln:point()+1)
    if string.find(c, "%u") then
	edln:delete()
	edln:insstr(string.lower(c))
    elseif string.find(c, "%l") then
	edln:delete()
	edln:insstr(string.upper(c))
    else
	edln:forward()
    end
end

function vim.decrement(edln)
    local c=string.sub(edln:contents(), edln:point()+1, edln:point()+1)
    if string.find(c, "%d") then
	edln:delete()
	c=c-1
	if c==-1 then c=9 end
	edln:insstr(tostring(c))
	edln:back()
    end
end

function vim.increment(edln)
    local c=string.sub(edln:contents(), edln:point()+1, edln:point()+1)
    if string.find(c, "%d") then
	edln:delete()
	c=c+1
	if c==10 then c=0 end
	edln:insstr(tostring(c))
	edln:back()
    end
end

vim.normal_mode()
vim.insert_mode()
