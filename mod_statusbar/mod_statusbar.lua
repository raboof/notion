--
-- ion/mod_statusbar/mod_statusbar.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
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


-- Settings etc. {{{

local default_tmpl=
string.format("[ %%date || %s: %%load || %s: %%mail_new/%%mail_total ]",
              TR("load"), TR("mail"))

-- Default settings
local settings={
    date_format='%a %Y-%m-%d %H:%M',
    template=default_tmpl,
    load_wtempl="x.xx, x.xx, x.xx",
}

local infowins={}


--DOC
-- Set format and update variables.
function mod_statusbar.set(s)
    settings=table.join(s, settings)
    local wt=mod_statusbar.get_w_template()
    for iw, _ in infowins do
        iw:set_natural_w(wt)
    end
end


--DOC
-- Set format and update variables.
function mod_statusbar.get()
    return table.copy(settings)
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

local function get_date()
    return os.date(settings.date_format)
end

function mod_statusbar.set_timer()
    local t=os.date('*t')
    local d=(60-t.sec)*1000
    
    timer:set(d, mod_statusbar.timer_handler)
end

function mod_statusbar.timer_handler(tmr)
    mod_statusbar.inform("date", get_date())
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


-- Status update {{{

local function process_template(fn)
    return string.gsub(settings.template, '%%(%%?[a-zA-Z0-9_]*)', 
                       function(s)
                           if string.sub(s, 1, 1)=='%' then
                               return s
                           elseif s=="" then
                               return ""
                           else
                               return fn(s)
                           end
                       end)
end

function mod_statusbar.get_status()
    return process_template(function(s)
                                return (meters[s] or "??")
                            end)
end

function mod_statusbar.get_w_template()
    return process_template(function(s)
                                local m=meters[s]
                                local w=settings[s.."_wtempl"]
                                return (w or m or "??")
                            end)
end


--DOC
-- Update statusbar contents. To be called after series
-- of \fnref{mod_statusbar.inform} calls.
function mod_statusbar.update()
    local s=mod_statusbar.get_status()
    local found=false
    
    for iw, _ in infowins do
        if not obj_exists(iw) then
            infowins[iw]=nil
        else
            iw:set_text(s)
            found=true
        end
    end
    
    return found
end

-- }}}


-- ion-statusd support {{{

local statusd_running=false

function mod_statusbar.rcv_statusd(str)
    local data=""

    local function doline(i)
        if i=="." then
            mod_statusbar.update()
        else
            local _, _, m, v=string.find(i, "^([^:]+):%s*(.*)")
            if m and v then
                mod_statusbar.inform(m, v)
            end
        end
    end
    
    while str do
        data=string.gsub(data..str, "([^\n]*)\n", doline)
        str=coroutine.yield()
    end
    
    ioncore.warn(TR("ion-statusd quit."))
    statusd_running=false
    meters={}
    mod_statusbar.update()
end


function mod_statusbar.launch_statusd()
    local function get_statusd_params()
        if settings.statusd_params then
            return settings.statusd_params
        else
            local mods={}
            process_template(function(s)
                                 local _, _, m = string.find(s, "^([^_]+)")
                                 if m then
                                     mods[m]=true
                                 end
                             end)
            local params=""
            table.foreach(mods, function(k) params=params.." -M "..k end)
            return params
        end
    end
    
    if statusd_running then
        return
    end
    
    local statusd=ioncore.lookup_script("ion-statusd")
    if not statusd then
        ioncore.warn(TR("Could not find %s", script))
    end
    
    local cmd=statusd.." "..get_statusd_params()
    local cr=coroutine.wrap(mod_statusbar.rcv_statusd)

    statusd_running=ioncore.popen_bgread(cmd, cr)
end

--}}}


-- Initialisation {{{

    
--DOC
-- Create a statusbar.
function mod_statusbar.create(param)
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
    
    local iw=scr:set_stdisp({
        type="WStatusBar", 
        pos=(param.pos or "bl"),
        name="*statusbar*",
    })
    
    if not iw then
        error(TR("Failed to create statusbar."))
    end

    mod_statusbar.init_timer()
    
    mod_statusbar.launch_statusd()
    
    infowins[iw]=true
    iw:set_natural_w(mod_statusbar.get_w_template())
    iw:set_text(mod_statusbar.get_status())
    
    return iw
end

-- }}}


-- Mark ourselves loaded.
_LOADED["mod_statusbar"]=true


-- Load user configuration file
dopath('cfg_statusbar', true)

