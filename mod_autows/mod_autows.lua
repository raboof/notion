--
-- ion/mod_autows/mod_autows.lua
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

if not ioncore.load_module("mod_autows") then
    return
end

local mod_autows=_G["mod_autows"]

local T={}
--_G.templates=T

-- Settings {{{

local S={}
-- Classes:
--  (T)erminal
--  (V)iewer
--  (M)isc
S.valid_classifications={["V"]=true, ["T"]=true, ["M"]=true,}

-- Xterm, rxvt, aterm, etc. all have "XTerm" as class part of WM_CLASS
S.terminal_emulators={["XTerm"]=true,} 

-- Pixel scale factor from 1280x1024/75dpi
S.scalef=1.0

--S.b_ratio=(1+math.sqrt(5))/2
--S.s_ratio=1
S.b_ratio=3
S.s_ratio=2

S.b_ratio2=7
S.s_ratio2=1

S.templates={}

S.templates["default"]={
    type="WSplitFloat",
    dir="horizontal",
    tls=S.b_ratio,
    brs=S.s_ratio,
    tl={
        type="WSplitPane",
        contents={
            type="WSplitFloat",
            dir="vertical",
            tls=S.b_ratio2,
            brs=S.s_ratio2,
            tl={
                type="WSplitPane",
                marker="V:single",
            },
            br={
                type="WSplitPane",
                marker="M:right",
            },
        },
    },
    br={
        type="WSplitPane",
        marker="T:up",
    },
}


--[[
S.templates["alternative1"]={
    type="WSplitSplit",
    dir="horizontal",
    tls=S.s_ratio2,
    brs=S.b_ratio2,
    marker="M:down;:",
    tl={},
    br={
        type="WSplitSplit",
        dir="vertical",
        tls=S.b_ratio,
        brs=S.s_ratio,
        marker="V:single;T:right",
        tl={},
        br={},
    },
}


S.templates["alternative2"]={
    type="WSplitSplit",
    dir="vertical",
    tls=S.b_ratio,
    brs=S.s_ratio,
    marker=":;T:right",
    tl={
        type="WSplitSplit",
        dir="horizontal",
        tls=S.s_ratio2,
        brs=S.b_ratio2,
        marker="M:down;V:single",
        tl={},
        br={},
    },
    br={},
}
]]

S.template=S.templates["default"]

S.shrink_minimum=32

-- }}}


-- Helper code {{{

local function sfind(s, p)
    local function drop2(a, b, ...)
        return unpack(arg)
    end
    return drop2(string.find(s, p))
end


function T.div_length(w, r1, r2)
    local a=math.ceil(w*r1/(r1+r2))
    return a, w-a
end

function T.split3(d, ls, cs, rs, lo, co, ro)
    return {
        tls = ls+cs,
        brs = rs,
        dir = d,
        tl = {
            tls = ls,
            brs = cs,
            dir = d,
            tl = (lo or {}),
            br = (co or {}),
        },
        br = (ro or {}),
    }
end

function T.center3(d, ts, cs, lo, co, ro)
    local sc=math.min(ts, cs)
    local sl=math.floor((ts-sc)/2)
    local sr=ts-sc-sl
    local r=T.split3(d, sl, sc, sr, lo, co, ro)
    return r
end

function T.split2(d, ts, ls, rs, lo, ro)
    if ls and rs then
        assert(ls+rs==ts)
    elseif not ls then
        ls=ts-rs
    else
        rs=ts-ls
    end
    assert(rs>=0 and ls>=0)
    return {
        type="WSplitSplit",
        dir=d,
        tls=math.floor(ls),
        brs=math.ceil(rs),
        tl=lo,
        br=ro,
    }
end

-- }}}


-- Classification {{{

function T.classify(ws, reg)
    if obj_is(reg, "WClientWin") then
        -- Check if there's a winprop override
        local wp=ioncore.getwinprop(reg)
        if wp and wp.autows_classification then
            if S.valid_classifications[wp.autows_classification] then
                return wp.autows_classification
            end
        end
        
        -- Handle known terminal emulators.
        local id=reg:get_ident()
        if S.terminal_emulators[id.class] then
            return "T"
        end
    end
    
    -- Try size heuristics.
    local cg=reg:geom()

    if cg.w<3/8*(1280*S.scalef) then
        return "M"
    end
    
    if cg.h>4/8*(960*S.scalef) then
        return "V"
    else
        return "T"
    end
end

-- }}}


-- Placement code {{{

function T.use_unused(p, n, d, forcefit)
    if d=="single" then
        p.res_node=n
        p.res_config={reg=p.frame}
        return true
    end

    -- TODO: Check fit
    local sg=n:geom()
    local fsh=p.frame:size_hints()
    local fg=p.frame:geom()
    
    if d=="up" or d=="down" then
        p.res_node=n
        if fsh.min_h>sg.h then
            return false
        end
        local fh=math.min(fg.h, sg.h)
        if d=="up" then
            p.res_config=T.split2("vertical", sg.h, nil, fh,
                                  {}, {reg=p.frame})
        else
            p.res_config=T.split2("vertical", sg.h, fh, nil,
                                  {reg=p.frame}, {})
        end
        return true
    elseif d=="left" or d=="right" then
        p.res_node=n
        if fsh.min_w>sg.w then
            return false
        end
        local fw=math.min(fg.w, sg.w)
        if d=="left" then
            p.res_config=T.split2("horizontal", sg.w, nil, fw,
                                  {}, {reg=p.frame})
        else
            p.res_config=T.split2("horizontal", sg.w, fw, nil,
                                  {reg=p.frame}, {})
        end
        return true
    end
end


function T.scan_pane(p, node, pdir)
    local function do_scan_active(n)
        local t=obj_typename(n)
        if t=="WSplitRegion" then
            p.res_node=n
            return true
        elseif t=="WSplitSplit" or t=="WSplitFloat" then
            local a, b=n:tl(), n:br()
            if b==n:current() then
                a, b=b, a
            end
            return (do_scan_active(a) or do_scan_active(b))
        end
    end

    local function do_scan_unused(n, d, forcefit)
        local t=obj_typename(n)
        if t=="WSplitSplit" or t=="WSplitFloat" then
            local sd=n:dir()
            if sd=="vertical" then
                if d=="up" then
                    return do_scan_unused(n:tl(), d, forcefit)
                elseif d=="down" then
                    return do_scan_unused(n:br(), d, forcefit)
                end
            elseif sd=="horizontal" then
                if d=="left" then
                    return do_scan_unused(n:tl(), d, forcefit)
                elseif d=="right" then
                    return do_scan_unused(n:br(), d, forcefit)
                end
            end
        elseif t=="WSplitUnused" then
            -- Found it
            return T.use_unused(p, n, d, forcefit)
        end
    end
    
    if do_scan_unused(node, pdir, false) then
        return true
    end
    
    if do_scan_active(node, pdir) then
        return true
    end

    return do_scan_unused(node, pdir, true)
end
    

function T.make_placement(p)
    if p.specifier then
        local n=p.specifier
        local pcls, pdir
        
        while n and not obj_is(n, "WSplitPane") do
            n=n:parent()
        end
        
        if n then
            pcls, pdir=sfind((n:marker() or ""), "(.*):(.*)")
        end
            
        return T.use_unused(p, p.specifier, (pdir or "single"), false)
    end
    
    local cls=T.classify(p.ws, p.reg)
    
    local function do_scan_cls(n)
        local t=obj_typename(n)
        if t=="WSplitPane" then
            local m=n:marker()
            if m then
                local pcls, pdir=sfind(m, "(.*):(.*)")
                if pcls and pcls==cls then
                    return T.scan_pane(p, n:contents(), pdir)
                end
            else
                return do_scan_cls(n:contents())
            end
        elseif t=="WSplitSplit" or t=="WSplitFloat" then
            return (do_scan_cls(n:tl()) or do_scan_cls(n:br()))
        end
    end
    
    local function do_scan_fallback(n)
        local t=obj_typename(n)
        if t=="WSplitUnused" then
            p.res_node=n
            p.res_config={reg=p.frame}
            return true
        elseif t=="WSplitRegion" then
            p.res_node=n
            return true
        elseif t=="WSplitPane" then
            return do_scan_fallback(n:contents())
        elseif t=="WSplitSplit" or t=="WSplitFloat" then
            return (do_scan_fallback(n:tl()) or do_scan_fallback(n:br()))
        end
    end
    
    local node=p.ws:split_tree()
    return (do_scan_cls(node) or do_scan_fallback(node))
end

-- }}}


-- Layout initialisation {{{


function T.calc_sizes(tmpl, sw, sh)
    tmpl._w, tmpl._h=sw, sh

    if tmpl.type=="WSplitSplit" or tmpl.type=="WSplitFloat" then
        local tmps, tlw, tlh, brw, brh
        -- Calculate pixel sizes of things
        if tmpl.dir=="vertical" then
            tmps=sh
        else
            tmps=sw
        end
        
        tmpl.tls, tmpl.brs=T.div_length(tmps, tmpl.tls, tmpl.brs)
    
        if tmpl.dir=="vertical" then
            tlw, brw=sw, sw
            tlh, brh=tmpl.tls, tmpl.brs
        else
            tlw, brw=tmpl.tls, tmpl.brs
            tlh, brh=sh, sh
        end
    
        T.calc_sizes(tmpl.tl, tlw, tlh)
        T.calc_sizes(tmpl.br, brw, brh)
    end
end


function T.init_layout(p)
    p.layout=table.copy(S.template, true) -- deep copy template
    local wg=p.ws:geom()
    T.calc_sizes(p.layout, wg.w, wg.h)
    return true
end

-- }}}


-- Initialisation {{{

function T.setup_hooks()
    local function hookto(hkname, fn)
        local hk=ioncore.get_hook(hkname)
        if not hk then
            error("No hook "..hkname)
        end
        if not hk:add(fn) then
            error("Unable to hook to "..hkname)
        end
    end

    hookto("autows_init_layout_alt", T.init_layout)
    hookto("autows_make_placement_alt", T.make_placement)
end


T.setup_hooks()

-- }}}


-- Mark ourselves loaded.
_LOADED["templates"]=true
