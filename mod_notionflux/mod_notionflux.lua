if package.loaded["mod_notionflux"] then return end

local mod_notionflux={}

-- this function is used by C to collect print() calls into a buffer
function mod_notionflux.output_collector()
    -- collecting strings in a table and using table.concat produces less garbage
    local out={}

    local function print(...)
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
    end

    local function finish()
        local ret=table.concat(out)
        out=nil
        return ret
    end

    return print, finish
end

-- takes input from mod_notionflux socket as a function (as produced by
-- loadstring), calls it with modified _G.print, returns a string containing all
-- printed text and the return value
function mod_notionflux.run_lua(fn)
    local newprint, finish = mod_notionflux.output_collector()
    local oldprint=_G.print
    _G.print=newprint
    local ok, res=pcall(fn)
    _G.print=oldprint
    local out=finish() -- swallowed in case of error
    if not ok then error(res) end

    -- format a string result using %q
    res = type(res) == "string" and string.format("%q", res) or tostring(res)
    out = #out>0 and out.."\n" or ""
    return out..res
end

_G["mod_notionflux"]=mod_notionflux
if not ioncore.load_module("mod_notionflux") then
    _G["mod_notionflux"]=nil
    return
end

package.loaded["mod_notionflux"]=true
