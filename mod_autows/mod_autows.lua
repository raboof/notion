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
S.s_ratio=1

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

function T.classify(ws, cwin)
    -- Check if there's a winprop override
    local wp=ioncore.getwinprop(cwin)
    if wp and wp.autows_classification then
        if S.valid_classifications[wp.autows_classification] then
            return wp.autows_classification
        end
    end
    
    -- Handle known terminal emulators.
    local id=cwin:get_ident()
    if S.terminal_emulators[id.class] then
        return "T"
    end
    
    -- Try size heuristics.
    local cg=cwin:geom()

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


-- Layout scan {{{


function T.scan_pane(p, node, pdir)
    local fsh=p.frame:size_hints()
    local fg=p.frame:geom()
    
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
    
    local function try_use_unused(n, d)
        -- TODO: Check fit
        local sg=n:geom()
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
            if d=="single" then
                p.res_node=n
                p.res_config={reg=p.frame}
                return true
            end
            return try_use_unused(n, d, forcefit)
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
    

function T.scan_layout(p, node)
    local cls=T.classify(p.ws, p.cwin)
    
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
    
    return (do_scan_cls(node) or do_scan_fallback(node))
end

-- }}}


-- Layout Initialisation {{{

function T.do_init_layout(p, cls, tmpl)
    local function split2_swap(d, ts, ls, rs, l, r, do_swap)
        if do_swap then
            ls, l, rs, r=rs, r, ls, l
        end
        return T.split2(d, ts, ls, rs, l, r)
    end
    
    local function do_place(dir, sw2, sh2)
        local fg=p.frame:geom()
        local fnode={reg=p.frame}
        if dir=="single" then
            return fnode, fg.w, fg.h
        elseif dir=="up" or dir=="down" then
            local fsz=math.min(sh2, fg.h)
            local split=split2_swap("vertical", sh2, fsz, nil, fnode, {},
                                    dir=="up")
            return split, fg.w, nil
        elseif dir=="left" or dir=="right" then
            local fsz=math.min(sw2, fg.w)
            local split=split2_swap("horizontal", sw2, fsz, nil, fnode, {},
                                    dir=="left")
            return split, nil, fg.h
        else
            error('Invalid direction in marker')
        end
    end
    
    local function limit_size(rq, avail)
        if rq>(avail-S.shrink_minimum) then
            rq=avail-S.shrink_minimum
            if rq<=0 then
                rq=avail/2
            end
        end
        return rq
    end

    -- Variables
    local found, wrq, hrq
    
    if tmpl.type=="WSplitPane" then
        if tmpl.marker then
            local panec, paned=sfind(tmpl.marker, "(.*):(.*)")
            if panec and panec==cls then
                tmpl.contents, wrq, hrq=do_place(paned, tmpl._w, tmpl._h)
                found=true
                -- Adjust size requests based on heuristics for classes
                if cls=="M" then
                    hrq, wrq=nil, nil
                elseif cls=="V" then
                    if hrq<tmpl._w then
                        hrq=nil
                    end
                    if wrq<tmpl._h then
                        wrq=nil
                    end
                end
            end
        else
            found, wrq, hrq=T.do_init_layout(p, cls, tmpl.contents)
        end
    elseif tmpl.type=="WSplitSplit" or tmpl.type=="WSplitFloat" then
        local found_where
        found, wrq, hrq=T.do_init_layout(p, cls, tmpl.tl)
        if found then
            found_where="tl"
        else
            found, wrq, hrq=T.do_init_layout(p, cls, tmpl.br)
            if found then
                found_where="br"
            end
        end

        if tmpl.dir=="vertical" and hrq then
            local th=tmpl._h
            local h=limit_size(hrq, th)
            if found_where=="tl" then
                tmpl.tls, tmpl.brs=h, th-h
            else
                tmpl.tls, tmpl.brs=th-h, h
            end
            hrq=nil
        elseif tmpl.dir=="horizontal" and wrq then
            local tw=tmpl._w
            local w=limit_size(wrq, tw)
            if found_where=="tl" then
                tmpl.tls, tmpl.brs=w, tw-w
            else
                tmpl.tls, tmpl.brs=tw-w, w
            end
            wrq=nil
        end
    end
    
    return found, wrq, hrq
end


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


function T.init_layout(p, n)
    local cls=T.classify(p.ws, p.cwin)
    local tmpl=table.copy(S.template, true) -- deep copy template
    local wg=p.ws:geom()
    T.calc_sizes(tmpl, wg.w, wg.h)
    p.res_node=n
    p.res_config=tmpl
    return T.do_init_layout(p, cls, tmpl, wg.w, wg.h)
end

-- }}}


-- Callback {{{

-- Layout initialisation hook
function T.layout_alt_handler(p)
    local n=p.ws:split_tree()
    local t=obj_typename(n)
    
    if t=="WSplitSplit" or t=="WSplitFloat" then
        if obj_typename(n:tl())=="WSplitST" then
            n=n:br()
        elseif obj_typename(n:br())=="WSplitST" then
            n=n:tl()
        end
        t=obj_typename(n)
    end
    
    if t=="WSplitUnused" then
        return T.init_layout(p, n)
    else
        return T.scan_layout(p, n)
    end
end

-- }}}


-- Initialisation {{{

function T.setup_hook()
    local hkname="autows_layout_alt"
    local hk=ioncore.get_hook(hkname)
    if not hk then
        error("No hook "..hkname)
    end
    
    if not hk:add(T.layout_alt_handler) then
        error("Unable to hook to "..hkname)
    end
end


T.setup_hook()        

-- }}}


-- Mark ourselves loaded.
_LOADED["templates"]=true
