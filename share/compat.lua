--
-- Compatibility functions to make transition from older (yet Lua-enabled)
-- versions of Ion easier. However, these definitions will gradually be
-- removed (say, about a month after addition) to force eventual conversion
-- of configuration files.
-- 

local function obsolete(name, fn)
    function obswrap(...)
        local fnx=fn
        if type(fn)=="string" then
            fnx=_G[fn]
        end
        warn("Warning: function " .. name .. " is obsolete.\n")
        return fnx(unpack(arg))
    end
    _G[name]=obswrap
end

-- Added 2003-04-22
obsolete("floatframe_raise", "region_raise")
obsolete("floatframe_lower", "region_lower")

-- Added 2003-05-02
local vertical_resize=true

obsolete("ionframe_resize_vert", function(frame)
                                     vertical_resize=true
                                     ionframe_begin_resize(frame)
                                 end
        )

obsolete("ionframe_resize_horiz", function(frame)
                                      vertical_resize=false
                                      ionframe_begin_resize(frame)
                                  end
        )

obsolete("ionframe_grow", function(frame)
                              if vertical_resize then
                                  ionframe_grow_vert(frame)
                              else
                                  ionframe_grow_horiz(frame)
                              end
                          end
        )

obsolete("ionframe_shrink", function(frame)
                                if vertical_resize then
                                    ionframe_shrink_vert(frame)
                                else
                                    ionframe_shrink_horiz(frame)
                                end
                            end
        )


-- Added 2003-05-07
obsolete("ionws_split", "ionframe_split")
obsolete("ionws_split_empty", "ionframe_split_empty")
obsolete("ionws_split_top", "ionws_newframe")

obsolete("make_screen_switch_nth_fn", function(n)
                                          local function call_nth(scr)
                                              screen_switch_nth_on_cvp(scr, n)
                                          end
                                          return call_nth
                                      end)

-- Added 2003-05-12
obsolete("region_get_x", function(reg) return region_geom(reg).x end)
obsolete("region_get_y", function(reg) return region_geom(reg).y end)
obsolete("region_get_w", function(reg) return region_geom(reg).w end)
obsolete("region_get_h", function(reg) return region_geom(reg).h end)


-- Added 2003-05-21
obsolete("region_set_w", function(reg, w) region_request_geom(reg, {w=w}) end)
obsolete("region_set_h", function(reg, h) region_request_geom(reg, {h=h}) end)

-- Added 2003-05-31
local new_floatframe_do_resize=floatframe_do_resize
local new_ionframe_do_resize=ionframe_do_resize

obsolete("ionframe_do_resize", function(f, x, y)
                                   new_ionframe_do_resize(f, x, x, y, y)
                               end)

obsolete("floatframe_do_resize", function(f, x, y)
                                     new_floatframe_do_resize(f, x, x, y, y)
                                 end)

-- Added 2003-06-09
obsolete("make_active_leaf_fn", 
         function(fn)
             if not fn then
                 error("fn==nil", 2)
             end
             return function(reg)
                        fn(region_get_active_leaf(reg))
                    end
         end)

-- Added 2003-06-17
obsolete("region_get_active_leaf",
         function(reg)
             local r=reg
             repeat
                 reg=r
                 r=region_active_sub(reg)
             until r==nil
             return reg
         end)

-- Added 2003-06-27
obsolete("exec_in_frame", exec_in)

-- Added 2003-06-27
obsolete("floatws_destroy", floatws_relocate_and_close)

-- Added 2003-07-08
obsolete("region_manage", mplex_attach)
obsolete("region_manage_new", mplex_attach_new)

-- Added 2003-07-14
local oldmt=getmetatable(_G)

if not oldmt then 
    oldmt={} 
end

local remaps={
    region=	"WRegion",
    rootwin=	"WRootWin",
    mplex=	"WMPlex",
    screen=	"WScreen",
    genframe=	"WGenFrame",
    ionframe=	"WIonFrame",
    floatframe=	"WFloatFrame",
    genws=	"WGenWS",
    ionws=	"WIonWS",
    floatws=	"WFloatWS",
    split=	"WSplit",
    input=	"WInput",
    wmsg=	"WMessage",
    wedln=	"WEdln",
}

local function class_fn_wrapper(tab, name)
    if type(name)~="string" then
        return nil
    end
    local st, en, clspart, rest=string.find(name, "^([^_]+)_(.*)$")
    if clspart and rest and remaps[clspart] then
        return tab[remaps[clspart]][rest]
    end
    if oldmt.__index then
        if type(oldmt.__index)=="function" then
            return oldmt.__index(tab, name)
        else
            return oldmt.__index[name]
        end
    else
        return nil
    end
end

setmetatable(_G, {__index=class_fn_wrapper, __newindex=oldmt.__newindex})

-- Added 2003-07-14
obsolete("exec_on_wm_display", "exec")

-- Added 2003-08-08
obsolete("close_sub_or_self", WRegion.close_sub_or_self)
