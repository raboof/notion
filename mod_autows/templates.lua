--
-- ion/mod_autows/templates.lua -- Templates for WAutoWS
-- 
-- Copyright (c) Tuomo Valkonen 2004.
-- 
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--


-- This is a slight abuse of the _LOADED variable perhaps, but library-like 
-- packages should handle checking if they're loaded instead of confusing 
-- the user with require/include differences.
if _LOADED["templates"] then return end

local T={}
_G.templates=T


-- Helper code {{{

function T.split3(d, ls, cs, rs, static_, co)
    return {
        split_tls = ls+cs,
        split_brs = rs,
        split_dir = d,
        static = static_,
        tl = {
            split_tls = ls,
            split_brs = cs,
            split_dir = d,
            static = static_,
            tl = { },
            br = co
        },
        br = { },
    }
end

function T.center3(d, ts, cs, static, co)
    local sc=math.min(ts, cs)
    local sl=math.floor((ts-sc)/2)
    local sr=ts-sc-sl
    return T.split3(d, sl, sc, sr, static, co)
end

-- }}}


-- Layout templates {{{

-- Default layout template. This will return a configuration as follows,
-- with 'U' denoting unused space (a SPLIT_UNUSED split) and 'R'
-- the initial frame.
-- 
-- UU|UUU|UU
-- UU|---|UU
-- UU|RRR|UU
-- UU|---|UU
-- UU|UUU|UU
-- 
function T.default_layout(ws, reg)
    local gw, gr=ws:geom(), reg:geom()
    return T.center3("horizontal", gw.w, gr.w, true,
                     T.center3("vertical", gw.h, gr.h, false,
                               { 
                                   type = obj_typename(reg),
                                   reference = reg,
                               }))
    
end

-- Extended default. Has additional zero-width/height extendable unused 
-- spaces blocks around the default layout.
function T.default_layout_ext(ws, reg)
    return T.center3("horizontal", 1, 1, true,
                     T.center3("vertical", 1, 1, true,
                               T.default_layout(ws, reg)))
end

-- }}}


-- Callbacks {{{

-- Layout initialisation hook
function T.init_layout_alt(p)
    p.config=T.default_layout(p.ws, p.reg)
    return (p.config~=nil)
end

-- }}}


-- Initialisation {{{

function T.setup_hook()
    local hkname="autows_init_layout_alt"
    local hk=ioncore.get_hook(hkname)
    if not hk then
        error("No hook "..hkname)
    end
    
    if not hk:add(T.init_layout_alt) then
        error("Unable to hook to "..hkname)
    end
end


T.setup_hook()        

-- }}}


-- Mark ourselves loaded.
_LOADED["templates"]=true
