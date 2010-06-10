-- Mark next mapped window to be attached to a specified object

local marked

function mark_for_attach(frame)
    marked=frame
end

local function copy(t)
    local ct={}
    for k, v in t do
         ct[k]=v
    end
    return ct
end

local orig_get_winprop=get_winprop

local function marked_get_winprop(cwin)
    local props=orig_get_winprop(cwin)
    if not marked then
        return props
    end
    local newprops=copy(props or {})
    newprops.target=marked:name()
    marked=nil
    return newprops
end

get_winprop=marked_get_winprop
