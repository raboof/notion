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


-- Penalty calculation {{{

local PENALTY_NEVER=2^32-1
local S={}

local gr=(1+math.sqrt(5))/2
S.l_ratio=gr
S.r_ratio=1
S.t_ratio=gr
S.b_ratio=1

S.fit_fit_offset=0
S.fit_activereg_offset=400
S.fit_inactivereg_offset=800

-- grow or shrink percentage 
S.vert_fit_penalty_scale={
    {  10, 9000},
    {  50, 1000},
    {  80,  100},
    { 100,    0},
    { 120,  100},
    { 200, 1000},
    { 500, 9000},
}

-- grow or shrink percentage 
S.horiz_fit_penalty_scale={
    {  10, 9000},
    {  50, 3000},
    {  80, 1000},
    { 100,    0},
    { 120, 1000},
    { 200, 3000},
    { 500, 9000},
}

-- slack pixels used 
S.slack_penalty_scale={
    {   0,    0},
    {  50,  200},
    { 200, 1000},
    { 200, PENALTY_NEVER},
}


-- percentage remaining in other side of split
S.waste_penalty_scale={
    {  10, PENALTY_NEVER},
    {  10, 9000},
    { 100,  100},
}


function T.interp1(scale, v)
    local offset=0
    local function acc(i, j)
        return (scale[offset+i] and scale[offset+i][j])
    end
    while true do
        assert(acc(1, 2));
        
        if v<=acc(1, 1) or not acc(2, 1) then
            return acc(1, 2)
        end
        
        if v<acc(2, 1) then
            if acc(1, 2)==PENALTY_NEVER or acc(2, 1)==PENALTY_NEVER then
                return PENALTY_NEVER;
            end
            
            return ((acc(2, 1)*(v-acc(1, 1))+acc(1, 2)*(acc(2, 1)-v))/
                    (acc(2, 1)-acc(1, 1)))
        end
        offset=offset+1
    end
end

function T.frac(ss, s)
    return math.floor(100*ss/math.max(1, s));
end

function T.valif(b, v1, v2)
    if b then return v1 else return v2 end
end

function T.add_p(p1, p2)
    if p1==PENALTY_NEVER or p2==PENALTY_NEVER then
        return PENALTY_NEVER
    else
        return p1+p2
    end
end

function T.vert_fit_penalty(ms, s, ss)
    local d=T.frac(ss, s)
   
    if ss<ms then
        return PENALTY_NEVER -- Should just penalise more
    end
    
    return T.add_p(T.interp1(S.vert_fit_penalty_scale, d),
                   S.fit_fit_offset)
end

function T.horiz_fit_penalty(ms, s, ss)
    local d=T.frac(ss, s)
   
    if ss<ms then
        return PENALTY_NEVER -- Should just penalise more
    end
    
    return T.add_p(T.interp1(S.horiz_fit_penalty_scale, d),
                   S.fit_fit_offset)
end

function T.slack_penalty(ms, s, ss, islack)
    local ds=ss-s;

    if ss<ms then
        return PENALTY_NEVER -- Should just penalise more.
    end
    
    if ds>=0 then
        return PENALTY_NEVER -- fit will do 
    end
    
    -- If there's enough "immediate" slack, don't penalise more
    -- than normal fit would penalise stretching ss to s.
    if -ds<=islack then
        return T.interp1(S.vert_fit_penalty_scale, T.frac(ss, s))
    end
    
    return T.interp1(S.slack_penalty_scale, -ds);
end


function T.waste_penalty(ms, s, ss)
    if s>=ss then        
        return PENALTY_NEVER
    end
    
    return T.interp1(S.waste_penalty_scale, T.frac(ss-s, s))
end


function T.vert_regfit_penalty(s, ss, cwin, reg)
    local res=T.interp1(S.vert_fit_penalty_scale, T.frac(ss, s))
    return T.add_p(res, T.valif(reg:is_active(),
                                S.fit_activereg_offset,
                                S.fit_inactivereg_offset))
end

function T.horiz_regfit_penalty(s, ss, cwin, reg)
    local res=T.interp1(S.horiz_fit_penalty_scale, T.frac(ss, s))
    return T.add_p(res, T.valif(reg:is_active(),
                                S.fit_activereg_offset,
                                S.fit_inactivereg_offset))
end


function T.combine(h, v)
    return T.valif(h==PENALTY_NEVER or v==PENALTY_NEVER,
                   PENALTY_NEVER,
                   math.sqrt(h^2+v^2))
                   --math.max(h, v))
end

-- }}}


-- Helper code {{{

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
    assert((not ls) or (not rs))
    if not ls then
        ls=ts-rs
    else
        rs=ts-ls
    end
    assert(rs>0 and ls>0)
    return {
        split_dir=d,
        split_tls=ls,
        split_brs=rs,
        tl=lo,
        br=ro,
    }
end

-- }}}


-- Layout templates {{{

-- Extended default. Has additional zero-width/height extendable unused 
-- spaces blocks around the default layout.
function T.layout_ext(ws, reg, lo)
    return T.center3("horizontal", 1, 1,
                     nil, 
                     T.center3("vertical", 1, 1, nil, lo, nil),
                     nil)
end


function T.div_length(w, r1, r2)
    return w*r1/(r1+r2), w*r2/(r1+r2)
end


function T.ratio_layout_1d(d, ss, fs, min_fs, max_fs, islack,
                           l_ratio, r_ratio, allowed, horiz, contents)
    local fp
    
    if horiz then
        fp=T.horiz_fit_penalty
    else
        fp=T.vert_fit_penalty
    end
    
    if fs>ss then
        local ps=T.slack_penalty(min_fs, fs, ss, islack)
        local pt=fp(min_fs, fs, ss)
        
        if pt<ps then
            return pt, contents, nil
        else
            return ps, contents, math.min(fs, ss+islack)
        end
    else
        local pw=T.waste_penalty(0, fs, ss)
        local pt=fp(min_fs, fs, ss)
        
        if pw<pt then
            -- Maybe other penalty scale?
            local sl, sr=T.div_length(ss, l_ratio, r_ratio)
            local pl=fp(min_fs, fs, sl)
            local pr=fp(min_fs, fs, sr)
            local la, ra=string.find(allowed, 'l'), string.find(allowed, 'r')
            
            if (pr<pl or not la) and ra then
                -- Put on right or bottom side of split
                return pw, T.split2(d, ss, nil, fs, { }, contents), nil
            elseif la then
                -- Put on left or top side of split
                return pw, T.split2(d, ss, fs, nil, contents, { }), nil
            end
        end
        
        -- Use all space
        return pt, contents, nil
    end
end


function T.find_parent(node, t)
    while true do
        local p=node:parent()
        if not p then
            return
        end
        
        if p:type()==t then
            return p, ((p:tl()==node and "l") or "r")
        end
        node=p
    end
end
        
        
function T.ratio_layout(p)
    local fnode={reg=p.frame}
    local fg=p.frame:geom()
    local sg=p.node:geom()
    local vp, vsplit, dest_h
    local hp, layout, dest_w
    local tr, br, lr, rr=S.t_ratio, S.b_ratio, S.l_ratio, S.r_ratio
    local hmay, vmay="lr", "lr"
    
    local par, parfrom=T.find_parent(p.node, "SPLIT_VERTICAL")
    if par then
        hmay=""
        vmay=((parfrom=="l" and "r") or "l")
    elseif p.node:parent() then
        lr, rr=rr, lr
        vmay="l" -- Top only.
    end
    
    -- TODO: 1, nil, 0 (min_fs, max_fs, islack)
    vp, vsplit, dest_h=T.ratio_layout_1d("vertical", sg.h, fg.h, 1, nil, 0,
                                         tr, br, vmay, false, fnode)
    hp, layout, dest_w=T.ratio_layout_1d("horizontal", sg.w, fg.w, 1, nil, 0,
                                         lr, rr, hmay, true, vsplit)
    
    p.penalty=T.combine(vp, hp)
    p.config=layout
    p.dest_h=dest_h
    p.dest_w=dest_w
    
    return true
end


-- }}}


-- Callbacks {{{

function T.handle_unused(p)
    return T.ratio_layout(p)
end

function T.handle_regfit(p)
    local cwg=p.frame:geom() -- Should use p.cwin:geom()
    local sg=p.node:geom() -- Should use WMPlex.managed_geom instead
    local r=p.node:reg()
    p.penalty=T.combine(T.horiz_regfit_penalty(cwg.w, sg.w, p.cwin, r),
                        T.vert_regfit_penalty(cwg.h, sg.h, p.cwin, r))
    return true
end

-- Layout initialisation hook
function T.layout_alt_handler(p)
    local t=p.node:type()
    if t=="SPLIT_REGNODE" then
        return T.handle_regfit(p)
    elseif t=="SPLIT_UNUSED" then
        return T.handle_unused(p)
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
