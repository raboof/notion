--
-- Ion ionws module configuration file
--

-- Bindings for the tiled workspaces (ionws). These should work on any 
-- object on the workspace.

defbindings("WIonWS", {
    bdoc("Split current frame vertically."),
    kpress(MOD1.."S", "WIonWS.split_at(_, _sub, 'bottom', true)"),
    
    bdoc("Go to frame above/below/right/left of current frame."),
    kpress(MOD1.."P", "WIonWS.goto_dir(_, 'above')"),
    kpress(MOD1.."N", "WIonWS.goto_dir(_, 'below')"),
    kpress(MOD1.."Tab", "WIonWS.goto_dir(_, 'right')"),
    submap(MOD1.."K", {
        kpress("Tab", "WIonWS.goto_dir(_, 'left')"),
        
        bdoc("Split current frame horizontally."),
        kpress("S", "WIonWS.split_at(_, _sub, 'right', true)"),
        
        bdoc("Destroy current frame."),
        kpress("X", "WIonWS.unsplit_at(_, _sub)"),
    }),
})


-- Frame bindings. These work in (Ion/tiled-style) frames. Some bindings
-- that are common to all frame types and multiplexes are defined in
-- ion-bindings.lua.

--defbindings("WIonFrame", {
--})

-- WFloatFrame context menu extras

if mod_menu then
    defctxmenu("WIonWS", {
        menuentry("Destroy frame", 
                  "WIonWS.unsplit_at(_, _sub)"),
        menuentry("Split vertically", 
                  "WIonWS.split_at(_, _sub, 'bottom', true)"),
        menuentry("Split horizontally", 
                  "WIonWS.split_at(_, _sub, 'right', true)"),
        menuentry("Flip root", "_:split_tree():flip()"),
        menuentry("Flip parent", "_:node_of(_sub):parent():flip()"),
        menuentry("Transpose root", "_:split_tree():transpose()"),
        menuentry("Transpose parent", "_:node_of(_sub):parent():transpose()"),
    })
end

