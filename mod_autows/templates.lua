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

-- Settings {{{

local S={}
S.valid_classifications={["W"]=true, ["TE"]=true, ["TB"]=true,}
-- Xterm, rxvt, aterm, etc. all have class XTerm
S.terminal_emulators={["XTerm"]=true,} 

S.b_ratio=(1+math.sqrt(5))/2
S.s_ratio=1

S.templates={}

S.templates["default"]={
    split_dir="horizontal",
    split_tls=S.b_ratio,
    split_brs=S.s_ratio,
    marker=":;TE:up",
    tl={
        split_dir="vertical",
        split_tls=S.b_ratio,
        split_brs=S.s_ratio/2,
        marker="W:single;TB:right",
        tl={},
        br={},
    },
    br={},
}

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
        split_tls = ls+cs,
        split_brs = rs,
        split_dir = d,
        tl = {
            split_tls = ls,
            split_brs = cs,
            split_dir = d,
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
        split_dir=d,
        split_tls=math.floor(ls),
        split_brs=math.ceil(rs),
        tl=lo,
        br=ro,
    }
end

-- }}}


-- Classification {{{

function T.classify(ws, cwin)
    -- Check if there's a winprop override
    local wp=ioncorelib.get_winprop(cwin)
    if wp and wp.autows_classification then
        if S.valid_classifications[wp.autows_classification] then
            return wp.autows_classification
        end
    end
    
    -- Handle known terminal emulators.
    local id=cwin:get_ident()
    if S.terminal_emulators[id.class] then
        return "TE"
    end
    
    -- Try size heuristics. We are assuming a relative large 
    -- screen here.
    local wg, cg=ws:geom(), cwin:geom()
    local l, r=T.div_length(wg.w, S.b_ratio, S.s_ratio)
    local t, b=T.div_length(wg.h, S.b_ratio, S.s_ratio)
    
    if cg.w>=l then
        if cg.h>b then
            return "W"
        end
    else
        if cg.w>=r/2 then
            return "TE"
        end
    end
    
    -- All odd-sized windows are considered tool-boxes
    return "TB"
end

-- }}}


-- Layout scan {{{


function T.scan_layout(p, node)
    local cls=T.classify(p.ws, p.cwin)
    local fg=p.frame:geom()
    local fsh=p.frame:size_hints()
    
    -- TODO: dock

    local function do_scan_active(n)
        local t=n:type()
        if t=="SPLIT_REGNODE" then
            p.res_node=n
            return true
        elseif t=="SPLIT_VERTICAL" or t=="SPLIT_HORIZONTAL" then
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
        local t=n:type()
        if t=="SPLIT_VERTICAL" then
            if d=="up" then
                return do_scan_unused(n:tl(), d, forcefit)
            elseif d=="down" then
                return do_scan_unused(n:br(), d, forcefit)
            end
        elseif t=="SPLIT_HORIZONTAL" then
            if d=="left" then
                return do_scan_unused(n:tl(), d, forcefit)
            elseif d=="right" then
                return do_scan_unused(n:br(), d, forcefit)
            end
        elseif t=="SPLIT_UNUSED" then
            -- Found it
            if d=="single" then
                p.res_node=n
                p.res_config={reg=p.frame}
                return true
            end
            return try_use_unused(n, d, forcefit)
        end
    end
    
    local function do_scan_cls(n, d)
        if do_scan_unused(n, d, false) then
            return true
        end
        if do_scan_active(n, d) then
            return true
        end
        return do_scan_unused(n, d, true)
    end
    
    local function do_scan(n)
        local t=n:type()
        if t=="SPLIT_VERTICAL" or t=="SPLIT_HORIZONTAL" then
            local m=n:get_marker()
            if m then
                local lt, ld, rt, rd=sfind(m, "(.*):(.*);(.*):(.*)")
                if lt and lt==cls then
                    if do_scan_cls(n:tl(), ld) then
                        return true
                    end
                    do_scan(n:br())
                    return false
                elseif rt and rt==cls then
                    if do_scan_cls(n:br(), rd) then
                        return true
                    end
                    do_scan(n:tl())
                    return false
                end
            end
            
            return (do_scan(n:tl()) or do_scan(n:br()))
        elseif not p.res_node then
            if t=="SPLIT_UNUSED" or t=="SPLIT_REGNODE" then
                p.res_node=n
            end
        end
    end

    return do_scan(node)
end

-- }}}


-- Layout templates {{{


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
    local found, found2, wrq, hrq
    
    -- Leaf?
    if not tmpl.split_dir then
        return
    end
    
    -- Check marker
    if tmpl.marker then
        local lt, ld, rt, rd=sfind(tmpl.marker, "(.*):(.*);(.*):(.*)")
        
        if lt and lt==cls then
            tmpl.tl, wrq, hrq=do_place(ld, tmpl._tlw, tmpl._tlh)
            found="tl"
        elseif rt and rt==cls then
            tmpl.br, wrq, hrq=do_place(rd, tmpl._brw, tmpl._brh)
            found="br"
        end
        
        -- Adjust size requests based on heuristics for classes
        if cls=="TB" then
            hrq, wrq=nil, nil
        elseif cls=="W" then
            if hrq<sh then
                hrq=nil
            end
            if wrq<sw then
                wrq=nil
            end
        end
    end

    -- Recurse
    if not found and tmpl.tl then
        found2, wrq, hrq=T.do_init_layout(p, cls, tmpl.tl)
        if found2 then
            found="tl"
        end
    end
    if not found and tmpl.br then
        found2, wrq, hrq=T.do_init_layout(p, cls, tmpl.br)
        if found2 then
            found="br"
        end
    end

    if tmpl.split_dir=="vertical" and hrq then
        local th=tmpl._tlh+tmpl._brh
        local h=limit_size(hrq, th)
        if found=="tl" then
            tmpl.split_tls, tmpl.split_brs=h, th-h
        else
            tmpl.split_tls, tmpl.split_brs=th-h, h
        end
        hrq=nil
    elseif tmpl.split_dir=="horizontal" and wrq then
        local tw=tmpl._tlw+tmpl._brw
        local w=limit_size(wrq, tw)
        if found=="tl" then
            tmpl.split_tls, tmpl.split_brs=w, tw-w
        else
            tmpl.split_tls, tmpl.split_brs=tw-w, w
        end
        wrq=nil
    end
    
    return (found~=nil), wrq, hrq
end


function T.calc_sizes(tmpl, sw, sh)
    local tmps
    
    -- Reached leaf?
    if not tmpl.split_dir then
        return
    end
    
    -- Calculate pixel sizes of things
    if tmpl.split_dir=="vertical" then
        tmps=sh
    else
        tmps=sw
    end
    
    tmpl.split_tls, tmpl.split_brs=T.div_length(tmps, 
                                                tmpl.split_tls, 
                                                tmpl.split_brs)
    
    if tmpl.split_dir=="vertical" then
        tmpl._tlw, tmpl._brw=sw, sw
        tmpl._tlh, tmpl._brh=tmpl.split_tls, tmpl.split_brs
    else
        tmpl._tlw, tmpl._brw=tmpl.split_tls, tmpl.split_brs
        tmpl._tlh, tmpl._brh=sh, sh
    end
    
    T.calc_sizes(tmpl.tl, tmpl._tlw, tmpl._tlh)
    T.calc_sizes(tmpl.br, tmpl._brw, tmpl._brh)
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
    local t=n:type()
    
    if t=="SPLIT_UNUSED" then
        -- Initialise layout
        return T.init_layout(p, n)
    elseif t=="SPLIT_REGNODE" then
        p.res_node=t
        p.res_config={reg=p.frame}
        return true
    elseif t=="SPLIT_VERTICAL" or t=="SPLIT_HORIZONTAL" then
        return T.scan_layout(p, n)
    end
    
    return false
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
