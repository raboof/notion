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
        io.stderr:write("Warning: function " .. name .. " is obsolete.\n")
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
