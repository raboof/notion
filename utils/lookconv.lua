--
-- A script to convert old Ion .lua look-conf:s to the new format.
-- 

function mk_bd_fn(what)
    return function(hl, sh, pad)
               _G[what]={
                   highlight_pixels=hl, 
                   shadow_pixels=sh, 
                   padding_pixels=pad,
               }
           end
end

function mk_cg_fn(what)
    return function(hl, sh, bg, fg)
               _G[what]={
                   highlight_colour=hl,
                   shadow_colour=sh,
                   foreground_colour=fg,
                   background_colour=bg,
               }
           end
end
    
function set_font(fnt)
    normal_font=fnt
end

function set_tab_font(fnt)
    tab_font=fnt
end

function set_term_font(fnt)
end

function enable_ion_bar_inside_frame(enable)
    bar_inside_border=enable
end

function set_ion_spacing(spc)
    spacing=spc
end

function set_background_color(col)
    background_colour=col
end

function enable_transparent_background(enable)
end

set_tab_border=mk_bd_fn("tab_border")
set_frame_border=mk_bd_fn("frame_border")
set_input_border=mk_bd_fn("input_border")

set_act_tab_colors=mk_cg_fn("act_tab_colours")
set_act_tab_sel_colors=mk_cg_fn("act_tab_sel_colours")
set_act_frame_colors=mk_cg_fn("act_frame_colours")
set_tab_colors=mk_cg_fn("tab_colours")
set_tab_sel_colors=mk_cg_fn("tab_sel_colours")
set_frame_colors=mk_cg_fn("frame_colours")
set_input_colors=mk_cg_fn("input_colours")

function set_selection_colors(bg, fg)    
    selection_fg=fg
    selection_bg=bg
end

local function print_table(pad, what)
    local t=_G[what]
    if not t then return end
    for k, v in t do
        if type(v)=="number" then
            print(string.format("%s%s = %d,", pad, k, v))
        else
            print(string.format("%s%s = %q,", pad, k, v))
        end
    end
end

local function print_cg(what)
    print_table('    ', what)
end

local function print_bd(what)
    print_table('    ', what)
end

local function print_font(what)
    if _G[what] then
        print(string.format('    font = %q,', _G[what]))
    end
end

local function print_extra_cg(spec, what)
    if not _G[what] then return end
    print(string.format('    de_substyle("%s", {', spec))
    print_table('        ', what)
    print('    }),')
end

local function output(nm)
    print('-- '..nm..' drawing engine configuration file for Ion.\n')
    print('if not gr_select_engine("de") then return end\n')
    print('de_reset()\n')
    
    -- Base style
    print('de_define_style("*", {')
    print_cg("tab_colours")
    print_bd("tab_border")
    print('    border_style = "elevated",')
    print_font("normal_font")
    print('    text_align = "center",')
    print('})\n')
    
    -- Frame
    print('de_define_style("frame", {')
    print('    based_on = "*",')
    print_cg("frame_colours")
    print_bd("frame_border")
    print_extra_cg("active", "act_frame_colours")
    print('})\n')
    
    -- Ionframe
    print('de_define_style("frame-ionframe", {')
    print('    based_on = "frame",')
    print('    border_style = "inlaid",')
    if frame_border then
        print(string.format('    padding_pixels = %d,', 
                            frame_border.padding_pixels/2))
    end
    if spacing then
        print(string.format('    spacing = %d,', spacing))
    end
    if bar_inside_border==false then
        print('    ionframe_bar_inside_border = false,')
    end
    print('})\n')
    
    -- Floatframe
    print('de_define_style("frame-floatframe", {')
    print('    based_on = "frame",')
    print('    border_style = "ridge"')
    print('})\n')
    
    -- Tabs
    print('de_define_style("tab", {')
    print('    based_on = "*",')
    print_font("tab_font")
    print_extra_cg("active-selected", "act_tab_sel_colours")
    print_extra_cg("active-unselected", "act_tab_colours")
    print_extra_cg("inactive-selected", "tab_sel_colours")
    print_extra_cg("inactive-unselected", "tab_colours")
    if _G["tab_colours"] then
        _G["tab_colours"].foreground_colour="#eeeeee";
        _G["tab_colours"].background_colour="#990000";
    end
        
    print('    text_align = "center",')
    print('})\n')
    
    -- Frame tabs
    print('de_define_style("tab-frame", {')
    print('    based_on = "tab",')
    print_extra_cg("*-*-*-*-activity", "tab_colours")
    print('})\n')
    
    -- Ionframe tabs
    if spacing then
        print('de_define_style("tab-frame-ionframe", {')
        print('    based_on = "tab-frame",')
        print(string.format('    spacing = %d,', spacing))
        print('})\n')
    end
    
    -- Menu tabs
    print('de_define_style("tab-menuentry", {')
    print('    based_on = "tab",')
    print('    text_align = "left",')
    print('})\n')
    
    print('de_define_style("tab-menuentry-big", {')
    print('    based_on = "tab-menuentry",')
    print('    font = "-*-helvetica-medium-r-normal-*-18-*-*-*-*-*-*-*",')
    print('    padding_pixels = 10,')
    print('})\n')
    
    -- Input
    print('de_define_style("input", {')
    print('    based_on = "*",')
    print_cg("input_colours")
    print_bd("input_border")
    print('    border_style = "elevated",')
    if input_colours then
        print('    de_substyle("cursor", {')
        print(string.format('        background_colour = %q,', 
                            input_colours.foreground_colour))
        print(string.format('        foreground_colour = %q,', 
                            input_colours.background_colour))
        print('    }),')
    end
    if selection_bg and selection_fg then
        print('    de_substyle("selection", {')
        print(string.format('        background_colour = %q,', selection_bg))
        print(string.format('        foreground_colour = %q,', selection_fg))
        print('    }),')
    end
    print_font("normal_font")
    print('})\n')
    print('gr_refresh()\n')
end


-- Main

if not arg[1] then
    print("Usage: lookconv look-foo.lua > look-foo-new.lua")
    return
end

dofile(arg[1])

if background_colour then
    if frame_colours then
        frame_colours.padding_colour=frame_colours.background_colour
        frame_colours.background_colour=background_colour
    end
end

output(arg[1])

