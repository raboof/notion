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

local PENALTY_NEVER=-1
local S={}

S.fit_fit_offset=0
S.fit_activereg_offset=100
S.fit_inactivereg_offset=500

-- grow or shrink percentage 
S.fit_penalty_scale={
    {  10, 9000},
    {  33, 1000},
    {  80,  100},
    { 100,    0},
    { 120,  100},
    { 200,  200},
    { 500, 9000},
}

-- slack pixels used 
S.slack_penalty_scale={
    {   0,    0},
    {  50,  200},
    { 200, 1000},
    { 200, PENALTY_NEVER},
}


-- pixels remaining in other side of split
S.split_penalty_scale={
    {  10, PENALTY_NEVER},
    {  10, 1000},
    { 100,    0},
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

function T.fit_penalty(ms, s, ss)
    local d=T.frac(ss, s)
   
    if ss<ms then
        return PENALTY_NEVER -- Should just penalise more
    end
    
    return T.add_p(T.interp1(S.fit_penalty_scale, d),
                   S.fit_fit_offset)
end

--#define hfit_penalty(S, CWIN, FRAME_SZH, SZ, U, DS) \
--    fit_penalty(FRAME_SZH->min_width, SZ, (S)->geom.w)
--#define vfit_penalty(S, CWIN, FRAME_SZH, SZ, U, DS) \
--    fit_penalty(FRAME_SZH->min_height, SZ, (S)->geom.h)


function T.slack_penalty(ms, s, ss, slack, islack)
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
        return T.interp1(S.fit_penalty_scale, T.frac(ss, s))
    end
    
    return T.interp1(S.slack_penalty_scale, -ds);
end

--#define hslack_penalty(S, CWIN, FRAME_SZH, SZ, U, DS) \
--    slack_penalty(FRAME_SZH->min_width, SZ, (S)->geom.w, (U)->tot_h, (U)->l+(U)->r)
--#define vslack_penalty(S, CWIN, FRAME_SZH, SZ, U, DS) \
--    slack_penalty(FRAME_SZH->min_height, SZ, (S)->geom.h, (U)->tot_v, (U)->t+(U)->b)


function T.split_penalty(ms, s, ss, slack)
    if s>=ss then
        return PENALTY_NEVER
    end
    
    if ss<ms then
        return PENALTY_NEVER -- Should just penalise more.
    end

    return T.interp1(S.split_penalty_scale, T.frac(ss-s, s))
end

--#define hsplit_penalty(S, CWIN, FRAME_SZH, SZ, U, DS) \
--    split_penalty(FRAME_SZH->min_width, SZ, (S)->geom.w, (U)->tot_h)
--#define vsplit_penalty(S, CWIN, FRAME_SZH, SZ, U, DS) \
--    split_penalty(FRAME_SZH->min_height, SZ, (S)->geom.h, (U)->tot_v)


function T.regfit_penalty(s, ss, cwin, reg)
    local res=T.interp1(S.fit_penalty_scale, T.frac(ss, s))
    return T.add_p(res, T.valif(reg:is_active(),
                                S.fit_activereg_offset,
                                S.fit_inactivereg_offset))
end

--#define hregfit_penalty(S, CWIN, SZ) \
--    regfit_penalty(SZ, (S)->geom.w, CWIN, (S)->u.reg)
--#define vregfit_penalty(S, CWIN, SZ) \
--    regfit_penalty(SZ, (S)->geom.h, CWIN, (S)->u.reg)


function T.combine(h, v)
    return T.valif(h==PENALTY_NEVER or v==PENALTY_NEVER,
                   PENALTY_NEVER,
                   math.max(h, v))
end

-- }}}


-- Helper code {{{

function T.set_static(t)
    t.static=true
    return t
end

function T.set_lazy(t)
    t.lazy=true
    return t
end

function T.id(t)
    return t
end

function T.split3(d, ls, cs, rs, lo, co, ro, sattr)
    if not sattr then sattr=T.id end
    return sattr{
        split_tls = ls+cs,
        split_brs = rs,
        split_dir = d,
        tl = sattr{
            split_tls = ls,
            split_brs = cs,
            split_dir = d,
            tl = (lo or {}),
            br = (co or {}),
        },
        br = (ro or {}),
    }
end

function T.center3(d, ts, cs, lo, co, ro, sattr)
    local sc=math.min(ts, cs)
    local sl=math.floor((ts-sc)/2)
    local sr=ts-sc-sl
    local r=T.split3(d, sl, sc, sr, lo, co, ro, sattr)
    return r
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
    return T.center3("horizontal", gw.w, gr.w, 
                     nil,
                     T.center3("vertical", gw.h, gr.h, 
                               nil,
                               { 
                                   type = "?",
                                   reference = reg, 
                               }, 
                               nil,
                               T.set_lazy), 
                     nil,
                     T.set_lazy)
end

-- Extended default. Has additional zero-width/height extendable unused 
-- spaces blocks around the default layout.
function T.default_layout_ext(ws, reg)
    return T.center3("horizontal", 1, 1,
                     nil,
                     T.center3("vertical", 1, 1,
                               nil,
                               T.default_layout(ws, reg),
                               nil,
                               T.set_static),
                     nil,
                     T.set_lazy)
end

-- }}}


-- Callbacks {{{

function T.handle_unused(p)
    -- Dummy implementation. To be rewritten.
    local fg=p.frame:geom()
    local sg=p.node:geom()
    if p.depth==0 then
        p.penalty=0
        p.config=T.default_layout(p.ws, p.frame)
    else
        p.penalty=T.combine(T.fit_penalty(0, fg.w, sg.w),
                            T.fit_penalty(0, fg.h, sg.h))
        p.config={
            reference=p.frame
        }
    end
    return true
end

function T.handle_regfit(p)
    local cwg=p.cwin:geom()
    local sg=p.node:geom() -- Should use WMPlex.managed_geom instead
    local r=p.node:reg()
    p.penalty=T.combine(T.regfit_penalty(cwg.w, sg.w, p.cwin, r),
                        T.regfit_penalty(cwg.h, sg.h, p.cwin, r))
    
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
