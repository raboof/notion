--
-- Ion ionws module configuration file
--

-- Workspace bindings for the tiled workspaces.
-- These should work on any object on the workspace.

ionws_bindings{
    kpress(DEFAULT_MOD .. "N", ionws_goto_below),
    kpress(DEFAULT_MOD .. "P", ionws_goto_above),
    kpress(DEFAULT_MOD .. "Tab", ionws_goto_right),
    submap(DEFAULT_MOD .. "K") {
        kpress("AnyModifier+Tab", ionws_goto_left),
    },
}

-- Frame bindings. These work in (Ion/tiled-style) frames.

include("common-frame-bindings.lua")
ionframe_bindings(common_frame_bindings())
ionframe_bindings(frame_query_bindings())

ionframe_bindings{
    kpress(DEFAULT_MOD .. "R", ionframe_begin_resize),
    kpress(DEFAULT_MOD .. "S",
           function(frame) ionframe_split(frame, "bottom") end),

    submap(DEFAULT_MOD .. "K"){
        kpress("X", ionframe_close),
        kpress("AnyModifier+S",
               function(frame) ionframe_split(frame, "right") end),
    },
    
    mclick("Button1", genframe_p_switch_tab, "tab"),
    mdrag("Button1", genframe_p_tabdrag, "tab"),
    mdrag("Button1", genframe_p_resize, "border"),
    
    mclick("Button2", genframe_p_switch_tab, "tab"),
    mdrag("Button2", genframe_p_tabdrag, "tab"),
    
    mdrag("Mod1+Button3", genframe_p_resize),
}


-- Some wrapper functions for resize mode bindings

local function ionframe_grow_vert(frame)
    ionframe_do_resize(frame, 0, 1)
end

local function ionframe_grow_horiz(frame)
    ionframe_do_resize(frame, 1, 0)
end

local function ionframe_shrink_vert(frame)
    ionframe_do_resize(frame, 0, -1)
end

local function ionframe_shrink_horiz(frame)
    ionframe_do_resize(frame, -1, 0)
end

-- Frame resize mode bindings

ionframe_moveres_bindings{
    kpress("AnyModifier+Escape", ionframe_cancel_resize),
    kpress("AnyModifier+Return", ionframe_end_resize),
    kpress("AnyModifier+Up", ionframe_grow_vert),
    kpress("AnyModifier+Down", ionframe_shrink_vert),
    kpress("AnyModifier+Right", ionframe_grow_horiz),
    kpress("AnyModifier+Left", ionframe_shrink_horiz),
    kpress("AnyModifier+P", ionframe_grow_vert),
    kpress("AnyModifier+N", ionframe_shrink_vert),
    kpress("AnyModifier+F", ionframe_grow_horiz),
    kpress("AnyModifier+B", ionframe_shrink_horiz),
}

