--
-- Bindings common to both ionws and floatws modules' frames.
--

if common_frame_bindings~=nil then
    return
end

function common_frame_bindings() 
    return {
        submap(DEFAULT_MOD.."K") {
            kpress("AnyModifier+N", genframe_switch_next),
            kpress("AnyModifier+P", genframe_switch_prev),
            kpress("AnyModifier+1", 
                   function(f) genframe_switch_nth(f, 0) end),
            kpress("AnyModifier+2",
                   function(f) genframe_switch_nth(f, 1) end),
            kpress("AnyModifier+3",
                   function(f) genframe_switch_nth(f, 2) end),
            kpress("AnyModifier+4",
                   function(f) genframe_switch_nth(f, 3) end),
            kpress("AnyModifier+5",
                   function(f) genframe_switch_nth(f, 4) end),
            kpress("AnyModifier+6",
                   function(f) genframe_switch_nth(f, 5) end),
            kpress("AnyModifier+7",
                   function(f) genframe_switch_nth(f, 6) end),
            kpress("AnyModifier+8",
                   function(f) genframe_switch_nth(f, 7) end),
            kpress("AnyModifier+9",
                   function(f) genframe_switch_nth(f, 8) end),
            kpress("AnyModifier+0",
                   function(f) genframe_switch_nth(f, 9) end),
            kpress("AnyModifier+H", genframe_maximize_horiz),
            kpress("AnyModifier+V", genframe_maximize_vert),
        },
    }
end    

function frame_query_bindings()
    if query_attachclient==nil then
        return {}
    end
    
    local f11key, f12key="F11", "F12"
    
    if os.execute('uname -s -p|grep "SunOS sparc" > /dev/null')==0 then
        print("common-frame-bindings.lua: Uname test reported SunOS; "
              .. "mapping F11=Sun36, F12=SunF37.")
        f11key, f12key="SunF36", "SunF37"
    end
    
    return {
        kpress(DEFAULT_MOD.."A", query_attachclient),
        kpress(DEFAULT_MOD.."G", query_gotoclient),
        
        kpress("F1",
               function(f) query_runwith(f, "Manual page:", "ion-man") end),
        kpress("F3", query_exec),
        kpress("Mod1+F3", query_commands),
        kpress("F4",
               function(f) query_runwith(f, "SSH to:", "ion-ssh") end),
        kpress("F5",
               function(f) query_runfile(f, "Edit file:", "ion-edit") end),
        kpress("F6",
               function(f) query_runfile(f, "View file:", "ion-view") end),
        kpress("F9", query_workspace),
        kpress("Mod1+F9", query_workspace_with),
        kpress(f11key,
               function(f)
                   query_yesno(f, "Restart Ion (y/n)?", restart_wm)
               end
              ),
        kpress(f12key,
               function(f)
                   query_yesno(f, "Exit Ion (y/n)?", exit_wm)
               end
              ),
    }
end
