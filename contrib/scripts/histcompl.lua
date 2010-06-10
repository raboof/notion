-- History completion support for the line editor

local function historyi()
    function gn(s)
	local i=s.i
	s.i=s.i+1
	return mod_query.history_get(i)
    end
    return gn, {i=0}
end

local function complhist(tocompl)
    local res={}
    local res2={}
    for str in historyi() do
	local s, e=string.find(str, tocompl, 1, true)
	if s then
	    table.insert(res2, str)
	end
	if s==1 and e>=1 then
	    table.insert(res, str)
	end
    end
    return (#res>0 and res) or res2
end

function history_completor(wedln)
    wedln:set_completions(complhist(wedln:contents()))
end

defbindings("WEdln", {
    kpress("Shift+Tab", history_completor),
})
