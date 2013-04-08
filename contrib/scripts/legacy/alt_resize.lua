-- Authors: Per Olofsson <pelle@dsv.su.se>
-- License: Public domain
-- Last Changed: Unknown
--
-- Alternative resizing for ion-devel
--
-- By Per Olofsson <pelle@dsv.su.se> (public domain)
--
--
-- More intuitive resizing keys for Ion:
--
-- key      function
-- --------------------------
-- left     shrink leftwards
-- right    grow rightwards
-- up       shrink upwards
-- down     grow downwards
--
-- The meaning of the corresponding keys are reversed when the window
-- is at the right or bottom edge of the workspace.
--
-- Add the code to ionws.lua to make use of it. You probably shouldn't
-- delete any bindings for keys that are not redefined here.
--

alt_resize={}

function alt_resize.resize(m, f, dir)
    local ws = f:manager()
    
    if dir == "left" then
        if ws:right_of(f) then
            m:resize( 0,-1, 0, 0)
        else
            m:resize( 1, 0, 0, 0)
        end
    elseif dir == "right" then
        if ws:right_of(f) then
            m:resize( 0, 1, 0, 0)
        else
            m:resize(-1, 0, 0, 0)
        end
    elseif dir == "up" then
        if ws:below(f) then
            m:resize( 0, 0, 0,-1)
        else
            m:resize( 0, 0, 1, 0)
        end
    elseif dir == "down" then
        if ws:below(f) then
            m:resize( 0, 0, 0, 1)
        else
            m:resize( 0, 0,-1, 0)
        end
    end
end

defbindings("WMoveresMode", {
    kpress("Left",  "alt_resize.resize(_, _sub, 'left' )"),
    kpress("Right", "alt_resize.resize(_, _sub, 'right')"),
    kpress("Up",    "alt_resize.resize(_, _sub, 'up'   )"),
    kpress("Down",  "alt_resize.resize(_, _sub, 'down' )"),
    kpress("F",     "alt_resize.resize(_, _sub, 'left' )"),
    kpress("B",     "alt_resize.resize(_, _sub, 'right')"),
    kpress("P",     "alt_resize.resize(_, _sub, 'up'   )"),
    kpress("N",     "alt_resize.resize(_, _sub, 'down' )"),
})

