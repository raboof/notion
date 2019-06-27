if package.loaded["mod_notionflux"] then return end
if not ioncore.load_module("mod_notionflux") then
  return
end

local mod_notionflux=_G.mod_notionflux
if not mod_notionflux then
    return
end

-- takes input from mod_notionflux socket as a function (as produced by
-- loadstring), calls it with modified _G.print, returns formatted return value
-- or forwards an error
function mod_notionflux.run_lua(fn, idx)
    local function newprint(...)
        -- collecting strings in a table and using table.concat produces less garbage
        local out = {}
        for i,v in ipairs{...} do
            if i>1 then
                table.insert(out, "\t")
            end
            local s=tostring(v)
            if type(s) ~= "string" then
                error "'tostring' must return a string to 'print'"
            end
            table.insert(out, s)
        end
        table.insert(out, "\n")

        mod_notionflux.xwrite(idx, table.concat(out))
    end
    local oldprint=_G.print
    _G.print=newprint
    local ok, res=pcall(fn)
    _G.print=oldprint
    if not ok then error(res) end

    -- format a string result using %q
    res = type(res) == "string"
        and string.format("%q", res)
        or  tostring(res)
    return res
end

package.loaded["mod_notionflux"]=true
