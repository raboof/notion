--
-- Ion ionws module configuration file
--

-- Bindings for the tiled workspaces (ionws). These should work on any 
-- object on the workspace.

defbindings("WIonWS", {
    kpress(MOD1.."N",           "WIonWS.goto_dir(_, 'below')"),
    kpress(MOD1.."P",           "WIonWS.goto_dir(_, 'above')"),
    kpress(MOD1.."Tab",         "WIonWS.goto_dir(_, 'right')"),
    submap(MOD1.."K", {
        kpress("AnyModifier+Tab", "WIonWS.goto_dir(_, 'left')"),
    }),
})


-- Frame bindings. These work in (Ion/tiled-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ion-bindings.lua.

defbindings("WIonFrame", {
    kpress(MOD1.."S",           "WIonFrame.split(_, 'bottom')"),
    submap(MOD1.."K", {
        kpress("AnyModifier+X", "WIonFrame.relocate_and_close(_)"),
        kpress("AnyModifier+S", "WIonFrame.split(_, 'right')"),
    }),
})
