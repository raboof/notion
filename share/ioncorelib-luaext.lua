--
-- ion/share/ioncorelib-luaext.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
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
        for k, v in t do
            local v2=v
            if deep and type(v)=="table" then
                if seen[v] then
                    error("Recursive table - unable to deepcopy")
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


function os.execute()
    warn("Do not use os.execute. Use ioncore.exec.")
end
