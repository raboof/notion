--
-- Ion floatws module configuration file
--

-- Workspace bindings for (PWM-style) floating frame workspaces.
-- These should work on any object on the workspace.

floatws_bindings{
    kpress(DEFAULT_MOD .. "Tab",
           function(ws)
               f=floatws_circulate(ws)
               region_raise(f)
           end
          ),
    submap(DEFAULT_MOD .. "K") {
        kpress("AnyModifier+Tab",
               function(ws)
                   f=floatws_backcirculate(ws)
                   region_raise(f)
               end
              ),
    }
}

-- Frame bindings. These work in (floating/PWM-style) frames.

include("common-frame-bindings.lua")
floatframe_bindings(common_frame_bindings())
floatframe_bindings(frame_query_bindings())

floatframe_bindings{
    kpress(DEFAULT_MOD .. "R", floatframe_begin_resize),
    
    mpress("Button1", region_raise, "tab"),
    mpress("Button1", region_raise, "border"),
    mclick("Button1", genframe_p_switch_tab, "tab"),
    mdrag("Button1", floatframe_p_move, "tab"),
    mdrag("Button1", genframe_p_resize, "border"),
    mdblclick("Button1", floatframe_toggle_shade, "tab"),
    
    mclick(DEFAULT_MOD .. "Button1", region_raise),
    mdrag(DEFAULT_MOD .. "Button1", floatframe_p_move),
    
    mclick("Button2", genframe_p_switch_tab, "tab"),
    mdrag("Button2", genframe_p_tabdrag, "tab"),
    
    mclick(DEFAULT_MOD .. "Button3", region_lower),
    mdrag(DEFAULT_MOD .. "Button3", genframe_p_resize),
}

-- Some wrapper functions for move/resize mode bindings

local function floatframe_grow_vert(frame)
    floatframe_do_resize(frame, 0, 1)
end

local function floatframe_grow_horiz(frame)
    floatframe_do_resize(frame, 1, 0)
end

local function floatframe_shrink_vert(frame)
    floatframe_do_resize(frame, 0, -1)
end

local function floatframe_shrink_horiz(frame)
    floatframe_do_resize(frame, -1, 0)
end

local function floatframe_move_down(frame)
    floatframe_do_move(frame, 0, 1)
end

local function floatframe_move_right(frame)
    floatframe_do_move(frame, 1, 0)
end

local function floatframe_move_up(frame)
    floatframe_do_move(frame, 0, -1)
end

local function floatframe_move_left(frame)
    floatframe_do_move(frame, -1, 0)
end

-- Frame move/resize mode bindings

floatframe_moveres_bindings{
    kpress("AnyModifier+Escape", floatframe_cancel_resize),
    kpress("AnyModifier+Return", floatframe_end_resize),
    kpress(DEFAULT_MOD .. "Up", floatframe_grow_vert),
    kpress(DEFAULT_MOD .. "Down", floatframe_shrink_vert),
    kpress(DEFAULT_MOD .. "Right", floatframe_grow_horiz),
    kpress(DEFAULT_MOD .. "Left", floatframe_shrink_horiz),
    kpress(DEFAULT_MOD .. "P", floatframe_grow_vert),
    kpress(DEFAULT_MOD .. "N", floatframe_shrink_vert),
    kpress(DEFAULT_MOD .. "F", floatframe_grow_horiz),
    kpress(DEFAULT_MOD .. "B", floatframe_shrink_horiz),
    kpress("Up", floatframe_move_up),
    kpress("Down", floatframe_move_down),
    kpress("Right", floatframe_move_right),
    kpress("Left", floatframe_move_left),
    kpress("P", floatframe_move_up),
    kpress("N", floatframe_move_down),
    kpress("F", floatframe_move_right),
    kpress("B", floatframe_move_left),
}

