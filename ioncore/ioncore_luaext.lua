--
-- ion/share/ioncore_luaext.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2008.
--
-- See the included file LICENSE for details.
--


--DOC
-- Make \var{str} shell-safe.
function string.shell_safe(str)
    return "'"..string.gsub(str, "'", "'\\''").."'"
end


--DOC
-- Make copy of \var{table}. If \var{deep} is unset, shallow one-level
-- copy is made, otherwise a deep copy is made.
function table.copy(t, deep)
    local function docopy(t, deep, seen)
        local nt={}
        for k, v in pairs(t) do
            local v2=v
            if deep and type(v)=="table" then
                if seen[v] then
                    error(TR("Recursive table - unable to deepcopy"))
                end
                seen[v]=true
                v2=docopy(v, deep, seen)
                seen[v]=nil
            end
            nt[k]=v2
        end
        return nt
    end
    return docopy(t, deep, deep and {})
end


--DOC
-- Add entries that do not exist in \var{t1} from \var{t2} to \var{t1}.
function table.append(t1, t2)
    for k, v in pairs(t2) do
        if t1[k]==nil then
            t1[k]=v
        end
    end
    return t1
end


--DOC
-- Create a table containing all entries from \var{t1} and those from
-- \var{t2} that are missing from \var{t1}.
function table.join(t1, t2)
    return table.append(table.copy(t1, false), t2)
end


--DOC
-- Insert all positive integer entries from t2 into t1.
function table.icat(t1, t2)
    for _, v in ipairs(t2) do
        table.insert(t1, v)
    end
    return t1
end


--DOC
-- Map all entries of \var{t} by \var{f}.
function table.map(f, t)
    local res={}
    for k, v in pairs(t) do
        res[k]=f(v)
    end
    return res
end


--DOC
-- Export a list of functions from \var{lib} into global namespace.
function export(lib, ...)
    for k, v in pairs(arg) do
        _G[v]=lib[v]
    end
end

