--
-- ion/mod_statusbar/mod_statusbar.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2005.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- This is a slight abuse of the _LOADED variable perhaps, but library-like 
-- packages should handle checking if they're loaded instead of confusing 
-- the user with require/include differences.
if _LOADED["mod_statusbar"] then return end

if not ioncore.load_module("mod_statusbar") then
    return
end

--local mod_menu=_G["mod_menu"]
--assert(mod_menu)
local mod_statusbar={}
_G["mod_statusbar"]=mod_statusbar


-- Settings {{{

-- Default settings
local defaults={
    date_format='%a %Y-%m-%d %H:%M',
    template=string.format(
        "[ %%date || %s: %%load || %s: %%mail_spool_new/%%mail_spool_total ]",
        TR("load"), TR("mail")
    ),
}

-- }}}


-- Statusbar list {{{

local statusbars={}

local function live_statusbars()
    return coroutine.wrap(function()
                              for sb, stng in statusbars do
                                  if obj_exists(sb) then
                                      coroutine.yield(sb, stng)
                                  else
                                      statusbars[sb]=nil
                                  end
                              end
                          end)
end
     
-- }}}


-- Meter list {{{

local meters={}

--DOC
-- Inform of a value.
function mod_statusbar.inform(name, value)
    meters[name]=value
end

-- }}}


-- Date meter {{{

local timer

function mod_statusbar.set_timer()
    local t=os.date('*t')
    local d=(60-t.sec)*1000
    
    timer:set(d, mod_statusbar.timer_handler)
end

function mod_statusbar.timer_handler(tmr)
    mod_statusbar.update()
    mod_statusbar.set_timer()
end

function mod_statusbar.init_timer()
    if not timer then
        timer=ioncore.create_timer()
        if not timer then
            error(TR("Failed to create a timer for statusbar."))
        end
    end
    
    if not timer:is_set() then
        mod_statusbar.timer_handler(timer)
    end
end


-- }}}


-- Template processing {{{

local function process_template(template, meter_f, text_f, stretch_f)
    local st, en, b, c, r
    
    while template~="" do
        st, en, b, r=string.find(template, '^(.-)%%(.*)')
    
        if not b then
            text_f(template)
            break
        else
            if b~="" then
                text_f(b)
            end
            template=r

            st, en, c, r=string.find(template, '^([ %%])(.*)')

            if c then
                if c==' ' then
                    stretch_f(c)
                else
                    text_f(c)
                end
                template=r
            else 
                st, en, c, b, r=string.find(template,
                                            '^([<|>]?)([a-zA-Z0-9_]+)(.*)')
                if b then
                    meter_f(b, c)
                    template=r
                end
            end
        end
    end
end



function mod_statusbar.get_template_table(stng)
    local res={}
    local m=meters --set_date(stng, meters)
    local aligns={["<"]=0, ["|"]=1, [">"]=2}
    
    process_template(stng.template,
                     -- meter
                     function(s, c)
                         table.insert(res, {
                             type=2,
                             meter=s,
                             align=aligns[c],
                             tmpl=meters[s.."_template"]
                         })
                     end,
                     -- text
                     function(t)
                         table.insert(res, {
                             type=1,
                             text=t
                         })
                     end,
                     -- stretch
                     function(t)
                         table.insert(res, {
                             type=3,
                             text=t
                         })
                     end)
    return res
end

-- }}}

-- Update {{{
-- 
local function set_date(stng, meters)
    local d=os.date(stng.date_format)
    meters["date"]=d
    return meters
end


--DOC
-- Update statusbar contents. To be called after series
-- of \fnref{mod_statusbar.inform} calls.
function mod_statusbar.update(update_template)
    local found=false
    
    for sb, stng in live_statusbars() do
        if update_template then
            sb:set_template(mod_statusbar.get_template_table(stng))
        end
        sb:update(set_date(stng, meters))
        found=true
    end
    
    return found
end

-- }}}


-- ion-statusd support {{{

local statusd_pid=0
local statusd_cfg=" -q "
local statusd_modules={}

function mod_statusbar.rcv_statusd(str)
    local data=""
    local updatenw=false

    local function doline(i)
        if i=="." then
            mod_statusbar.update(updatenw)
        else
            local _, _, m, v=string.find(i, "^([^:]+):%s*(.*)")
            if m and v then
                mod_statusbar.inform(m, v)
                updatenw=updatenw or string.find(m, "_template")
            end
        end
    end
    
    while str do
        data=string.gsub(data..str, "([^\n]*)\n", doline)
        str=coroutine.yield()
    end
    
    ioncore.warn(TR("ion-statusd quit."))
    statusd_pid=0
    meters={}
    mod_statusbar.update(updatenw)
end


local function get_statusd_params()
    local mods={}
    for sb, stng in live_statusbars() do
        process_template(stng.template,
                         -- meter
                         function(s)
                             local _, _, m = string.find(s, "^([^_]+)")
                             if m then
                                 mods[m]=true
                             end
                         end,
                         -- text
                         function() 
                         end,
                         -- stretch
                         function()
                         end)
    end
    local params=statusd_cfg
    table.foreach(mods, function(k) params=params.." -m "..k end)
    return params
end


function mod_statusbar.cfg_statusd(cfg)
    --TODO: don't construct file name twice.
    ioncore.write_savefile("cfg_statusd", cfg)
    return ioncore.get_savefile("cfg_statusd")
end


--DOC
-- Launch ion-statusd with configuration table \var{cfg}.
function mod_statusbar.launch_statusd(cfg)
    if statusd_pid>0 then
        return
    end
    
    local statusd=ioncore.lookup_script("ion-statusd")
    if not statusd then
        ioncore.warn(TR("Could not find %s", script))
        return
    end

    local cfg=mod_statusbar.cfg_statusd(cfg or {})
    
    local cmd=statusd.." -c "..cfg..get_statusd_params()
    local cr=coroutine.wrap(mod_statusbar.rcv_statusd)

    statusd_pid=ioncore.popen_bgread(cmd, cr)
end

--}}}


-- Initialisation and default settings {{{

--DOC 
-- Set defaults. For Backwards compatibility.
function mod_statusbar.set(t)
    defaults=table.join(t, defaults)
end


--DOC 
-- Get defaults. For Backwards compatibility.
function mod_statusbar.get(t)
    return defaults
end

    
--DOC
-- Create a statusbar.
function mod_statusbar.create(param_)
    local param=table.join(param_, defaults)
    
    local scr=ioncore.find_screen_id(param.screen or 0)
    if not scr then
        error(TR("Screen not found."))
    end
    
    if not param.force then
        local stdisp=scr:get_stdisp()
        if stdisp and stdisp.reg then
            error(TR("Screen already has an stdisp. Refusing to create a "..
                     "statusbar."))
        end
    end
    
    local sb=scr:set_stdisp({
        type="WStatusBar", 
        pos=(param.pos or "bl"),
        name="*statusbar*",
    })
    
    if not sb then
        error(TR("Failed to create statusbar."))
    end

    mod_statusbar.init_timer()
    
    statusbars[sb]=param
    sb:set_template(mod_statusbar.get_template_table(param))
    sb:update(set_date(param, meters))    
    
    return sb
end

-- }}}


-- Mark ourselves loaded.
_LOADED["mod_statusbar"]=true


-- Load user configuration file
dopath('cfg_statusbar', true)

-- Launch statusd if the user didn't launch it.
if statusd_pid<=0 then
    mod_statusbar.launch_statusd()
end
