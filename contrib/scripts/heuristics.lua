--
-- Window placement heuristics for WIonWS:s
-- 
-- (c) Tuomo Valkonen 2003.
-- 

--
-- In calculating penalties, the area of client window not overlapped by a 
-- frame is multiplied by 
--     heuristics.shrink_penalty,
-- the area of frame not overlapped by a client window is multiplied by
--     heuristics.splurge_penalty,
-- and the distance from the center of current frame to the frame under
-- consideration is multiplied by
--     heuristics.dist_penalty.
-- If there is no current frame, this value is taken to be zero. These three
-- values are summed to obtain the final penalty and the frame with the
-- smallest penalty is chosen. 
-- 
-- To disable heuristics for a particular window (class), set
--     ignore_heuristics
-- in a matching winprop.
-- 
-- To use this code to place windows with user given position in 
-- the frame they mostly overlap, set 
--     heuristics.userpos_by_overlap=true. 
-- and, to disable other heuristics ,
--     heuristics.only_overlap=true.
-- 

if not heuristics then
  heuristics = {
    shrink_penalty=20,
    splurge_penalty=1,
    dist_penalty=1500,
    userpos_by_overlap=true,
    only_overlap=false,
  }
end

-- Some helper functions {{

-- Returns an iterator that goes through the list L returning two values
-- at a time with step 1 between calls.
local function gettwo(L)
    return coroutine.wrap(function()
                              for i=1,table.getn(L)-1 do
                                  coroutine.yield(L[i], L[i+1])
                              end
                          end)
end

local function l2metric(a, b)
    return math.sqrt((a.x-b.x)^2+(a.y-b.y)^2)
end

local function geom_center(g)
    return {x=g.x+g.w/2, y=g.y+g.h/2}
end

local function geom2rect(geom)
    return {x1=geom.x, y1=geom.y, x2=geom.x+geom.w, y2=geom.y+geom.h}
end

local function geom2rect0(geom)
    return {x1=0, y1=0, x2=geom.w, y2=geom.h}
end

local function rect_within(C, R)
    return (R.x1>=C.x1 and R.x2<=C.x2 and R.y1>=C.y1 and R.y2<=C.y2)
end

local function rect_area(A)
    return (A.x2-A.x1)*(A.y2-A.y1)
end

-- }}}


-- Penalty calculation {{ 

-- Divide the (rectangular) area spanned by two rectangles into nine smaller
-- rectangles so that each of these smaller rectangles either are fully 
-- overlapped or not overlapped at all by each of the original rectangles
-- $A$ and $B$. For example:
-- \begin{verbatim}
--  AAAA       11123
--  AAABB      44456
--  AAABB  =>  44456
--     BB      77789
-- \end{verbatim}
local function divide_rectangles(A, B)
    local rects={}
    local X={A.x1, A.x2, B.x1, B.x2}
    local Y={A.y1, A.y2, B.y1, B.y2}
    table.sort(X)
    table.sort(Y)
    for x1, x2 in gettwo(X) do
        for y1, y2 in gettwo(Y) do
            local R={x1=x1, x2=x2, y1=y1, y2=y2}
            if rect_within(A, R) then R.a=1 else R.a=0 end
            if rect_within(B, R) then R.b=1 else R.b=0 end
            table.insert(rects, R)
        end
    end
    return rects
end

-- Calculate $\int w \circ (\chi_A-\chi_B) d\lambda$ for rectangles $A$
-- and $B$ and some $w$ with domain $\{-1, 0, 1\}$.
local function rect_penalty(A, B, w)
    local p=0
    local rects=divide_rectangles(A, B)
    for _, R in rects do
        p=p+rect_area(R)*w[R.a-R.b]
    end
    return p
end

-- Calculate the penalty for placing \var{cwin} in \var{frame} 
-- current frame's (if any) center being \var{center}.n
-- The penalty is given by
-- \[
--   d(A, B)+\text{dist penalty}*d_2(\text{center}, \text{center of frame})
-- \]
-- where $d$ is given by the above function with
-- \[
-- 	w(x)=\begin{cases}
-- 		\text{shrink penalty}, & x=-1, \\
-- 		0, x=0, \\
-- 		\text{splurge penalty}, & x=1. \\
-- 	     \end{cases}
-- \]
-- If \var{userpos} is set, then the geometries of \var{frame} and \var{cwin}
-- are used for $A$ and $B$, respectively. Otherwise $A$ and $B$ are 
-- rectangles with $(x, y)=(0, 0)$ and width and height given by \var{frame}'s 
-- and \var{cwin}'s geometries.
function heuristics.penalty(frame, cwin, userpos, center)
    local w={ 
        [-1] = heuristics.shrink_penalty, 
        [0]  = 0,
        [1]  = heuristics.splurge_penalty
    }

    local p=rect_penalty(geom2rect0(frame:geom()), geom2rect0(cwin:geom()), w)
    
    if center then
        local c2=geom_center(frame:geom())
        p=p+l2metric(center, c2)*heuristics.dist_penalty
    end
    
    return p
end

-- Calculate the area of \var{cwin} not overlapped by \var{frame}.
function heuristics.nonoverlap_penalty(frame, cwin)
    local w={[-1] = 1, [0] = 0, [1] = 0}
    return rect_penalty(geom2rect(frame:geom()), geom2rect(cwin:geom()), w)
end

-- }}}


-- Interface {{{

function heuristics.get_frame(ws, cwin, userpos)
    if not userpos and heuristics.only_overlap then
        return nil
    end
    
    local wp=get_winprop(cwin)
    if wp and wp.ignore_heuristics then
        return nil
    end
    
    local frames=ws:managed_list()
    local current=ws:current()
    local minpen, minframe, pen, center
    
    if current then
        local g=current:geom()
        center=geom_center(current:geom())
    end
    
    for _, frame in frames do 
        if userpos and heuristics.userpos_by_overlap then
            pen=heuristics.nonoverlap_penalty(frame, cwin)
        else
            pen=heuristics.penalty(frame, cwin, userpos, center)
        end
        
        if not minframe or pen<minpen then
            minframe=frame
            minpen=pen
        end
    end

    return minframe
end

-- }}}


-- Initialisation {{{

if not heuristics.saved_ionws_placement_method then
    heuristics.saved_ionws_placement_method=ionws_placement_method
    ionws_placement_method=heuristics.get_frame
end

-- }}}

