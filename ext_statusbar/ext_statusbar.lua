--
-- ion/ext_statusbar/ext_statusbar.lua
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
if _LOADED["ext_statusbar"] then return end

local ext_statusbar={}
_G["ext_statusbar"]=ext_statusbar


-- Settings etc. {{{

local default_tmpl=
string.format("[ %%date || %s: %%load || %s: %%mail_new/%%mail_total ]",
              TR("load"), TR("mail"))

-- Default settings
local settings={
    date_format='%Y-%m-%d %H:%M',
    template=default_tmpl,
    statusd_params="-m mail -m load",
}

local infowins={}


--DOC
-- Set format and update variables.
function ext_statusbar.set(s)
    settings=table.join(s, settings)
    local wt=ext_statusbar.get_w_template()
    for iw, _ in infowins do
        iw:set_natural_w(wt)
    end
end


--DOC
-- Set format and update variables.
function ext_statusbar.get()
    return table.copy(settings)
end

-- }}}


-- Meter list {{{

local meters={}
local wtempls={}

--DOC
-- Inform of a value.
function ext_statusbar.inform(name, value, wtempl)
    meters[name]=value
    wtempls[name]=wtempl
end

-- }}}


-- Date meter {{{

local timer

local function get_date()
    return os.date(settings.date_format)
end

function ext_statusbar.set_timer()
    local t=os.date('*t')
    local d=(60-t.sec)*1000
    
    timer:set(d, ext_statusbar.timer_handler)
end

function ext_statusbar.timer_handler(tmr)
    ext_statusbar.inform("date", get_date())
    if ext_statusbar.update() then
        ext_statusbar.set_timer()
    end
end

function ext_statusbar.init_timer()
    if not timer then
        timer=ioncore.create_timer()
        if not timer then
            error(TR("Failed to create a timer for statusbar."))
        end
    end
    
    if not timer:is_set() then
        ext_statusbar.timer_handler(timer)
    end
end


-- }}}


-- Status update {{{

function ext_statusbar.get_status()
    return string.gsub(settings.template, '%%([a-zA-Z0-9_]*)', 
                       function(s)
                           if s=="" then
                               return ""
                           end
                           return (meters[s] or "??")
                       end)
end

function ext_statusbar.get_w_template()
    return string.gsub(settings.template, '%%([a-zA-Z0-9_]*)', 
                       function(s)
                           if s=="" then
                               return ""
                           end
                           local m=meters[s]
                           local w=wtempls[s]
                           return (w or m or "??")
                       end)
end

function ext_statusbar.update()
    local s=ext_statusbar.get_status()
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

function ext_statusbar.rcv_statusd(str)
    local data=""

    local function doline(i)
        if i=="." then
            ext_statusbar.update()
        else
            local _, _, m, v=string.find(i, "^([^:]+):%s*(.*)")
            if m and v then
                ext_statusbar.inform(m, v)
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
    ext_statusbar.update()
end


function ext_statusbar.launch_statusd()
    if statusd_running then
        return
    end
    
    local statusd=ioncore.lookup_script("ion-statusd")
    if not statusd then
        ioncore.warn(TR("Could not find %s", script))
    end
    
    local cmd=statusd.." "..settings.statusd_params
    local cr=coroutine.wrap(ext_statusbar.rcv_statusd)
    
    statusd_running=ioncore.popen_bgread(cmd, cr)
end

--}}}


-- Initialisation {{{

    
--DOC
-- Create a statusbar.
function ext_statusbar.create(param)
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
        type="WInfoWin", 
        pos=(param.pos or "bl"),
        name="*statusbar*",
        style="stdisp-statusbar"
    })
    
    if not iw then
        error(TR("Failed to create statusbar."))
    end
    
    infowins[iw]=true
    iw:set_natural_w(ext_statusbar.get_w_template())
    iw:set_text(ext_statusbar.get_status())
    
    ext_statusbar.init_timer()
    
    ext_statusbar.launch_statusd()
    
    return iw
end

-- }}}


-- Mark ourselves loaded.
_LOADED["ext_statusbar"]=true


-- Load user configuration file
dopath('cfg_statusbar', true)

