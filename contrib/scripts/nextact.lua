-- Go to first found region with activity flag set.

local function filteri(f, l)
    local res={}
    for _, v in ipairs(l) do
        if f(v) then
            table.insert(res, v)
        end
    end
    return res
end

function goto_nextact()
    local l=filteri(WRegion.is_activity, ioncore.clientwin_list())
    if l[1] then
        l[1]:goto()
    end
end
