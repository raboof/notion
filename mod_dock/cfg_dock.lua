--
-- Ion-devel dock module configuration
--

-- The following settings only apply if no dock exists

local dock_type='none' -- embedded | floating | none
local dock_screen=0
local dock_params={
    grow="right", -- growth direction: left|right|up|down
    pos="bl",     -- position: embedded: tl|tr|bl|br, 
                  --           floating: also tc|bc|ml|mc|mr
    is_auto=true, -- whether new dockapps should be added automatically
}


--
-- Code to set up a dock
-- 

if dock_type~='none' then
    local function create_dock_if_missing()
        local f
        local d=ioncore.region_list("WDock")
        if table.getn(d)>0 then
            return
        end
        
        local scr=ioncore.find_screen_id(dock_screen)
        
        if dock_type=='embedded' then
            scr:set_stdisp(table.join(dock_params, {type="WDock"}))
        elseif dock_type=='floating' then
            scr:attach_new(table.join(dock_params, {
                type="WDock", 
                layer=2, 
                passive=true,
                switchto=true,
            }))
        else
            warn("Invalid dock type")
        end
    end

    local hk=ioncore.get_hook("ioncore_post_layout_setup_hook")
    hk:add(create_dock_if_missing)
end

if dock_type=='floating' then
    function toggle_floating_dock(r)
        local l=r:llist(2)
        for k, v in l do
            if obj_is(v, "WDock") then
                if v:is_mapped() then
                    r:l2_hide(v)
                else
                    r:l2_show(v)
                end
            end
        end
    end
    
    defbindings("WScreen", {
        kpress(MOD1.."space", "toggle_floating_dock(_)")
    })
end


--
-- Dock settings menu
-- 

dopath('mod_menu')

defmenu("dock-settings", {
    menuentry("Pos-TL", "_:set{pos='tl'}"),
    menuentry("Pos-TR", "_:set{pos='tr'}"),
    menuentry("Pos-BL", "_:set{pos='bl'}"),
    menuentry("Pos-BR", "_:set{pos='br'}"),
    menuentry("Grow-L", "_:set{grow='left'}"),
    menuentry("Grow-R", "_:set{grow='right'}"),
    menuentry("Grow-U", "_:set{grow='up'}"),
    menuentry("Grow-D", "_:set{grow='down'}"),
})

defbindings("WDock", {
    mpress("Button3", "mod_menu.pmenu(_, _sub, 'dock-settings')"),
})

