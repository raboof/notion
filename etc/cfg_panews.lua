--
-- Ion panews module configuration file
--

-- Bindings for unused area. 

defbindings("WUnusedWin", {
    bdoc("Begin move/resize mode."),
    kpress(MOD1.."R", "WUnusedWin.begin_kbresize(_)"),
    
    bdoc("Resize the area."),
    mdrag(MOD1.."Button3", "WUnusedWin.p_resize(_)"),
    mdrag(MOD1.."Button1", "WUnusedWin.p_move(_)"),
})


mod_panews.set{ 
    -- Layout template may be one of default|alternative1|alternative2 
    -- or a template table. (The one for 'default' is reproduced below
    -- as an example.)
    template="alternative1",
    -- The scale factor parameter controls the size-based classification 
    -- heuristic. The default of 1.0 is designed for 1280x1024 at 75dpi.
    --scalef=1.0,
}


-- The layout template for the 'default' layout looks as follows.
--[[
{
    type="WSplitFloat",
    dir="horizontal",
    tls=settings.b_ratio,
    brs=settings.s_ratio,
    tl={
        type="WSplitPane",
        contents={
            type="WSplitFloat",
            dir="vertical",
            tls=settings.b_ratio2,
            brs=settings.s_ratio2,
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
--]]
