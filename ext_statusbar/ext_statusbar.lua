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

local default_update_interval=10

-- Default settings
local settings={
    date_format='%Y-%m-%d %H:%M',
    update_interval=default_update_interval,
    mail_interval=60,
    template="[ %date || load: %load || mail: %mail_new/%mail_total ]"
}

local infowins={}
local timer=nil


--DOC
-- Set format and update variables.
function ext_statusbar.set(s)
    settings=table.join(s, settings)
    if settings.update_interval<=0 then
        settings.update_interval=default_update_interval
    end
    
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

--DOC
-- Register a new status meter.
function ext_statusbar.register_meter(name, fn, width_template)
    meters[name]={fn=fn, width_template=width_template}
end

-- }}}


-- Date meter {{{

local function get_date()
    return os.date(settings.date_format)
end

ext_statusbar.register_meter('date', get_date)

-- }}}


-- Load meter {{{

local function get_load_proc()
    local f=io.open('/proc/loadavg', 'r')
    if not f then
        return ""
    end
    local s=f:read('*l')
    f:close()
    local st, en, load=string.find(s, '^(%d+%.%d+ %d+%.%d+ %d+%.%d+)')
    return string.gsub((load or ""), " ", ", ")
end

local function get_load_uptime()
    local f=io.popen('uptime', 'r')
    if not f then
        return "??"
    end
    local s=f:read('*l')
    f:close()
    local st, en, load=string.find(s, 'load average: (%d+%.%d+, %d+%.%d+, %d+%.%d+)')
    return (load or "")
end

local function detect_load_fn()
    if get_load_proc()~="" then
        return get_load_proc
    else
        return get_load_uptime
    end
end

ext_statusbar.register_meter('load', detect_load_fn(), 'xx.xx xx.xx xx.xx')

-- }}}


-- Mail meters {{{

function calcmail(fname)
    local f=io.open(fname, 'r')
    local total, read, old=0, 0, 0
    local had_blank=true
    local in_headers=false
    local had_status=false
    
    if not f then
        return 0, 0, 0
    end
    
    for l in f:lines() do
        if had_blank and string.find(l, '^From ') then
            total=total+1
            had_status=false
            in_headers=true
            had_blank=false
        else
            had_blank=false
            if l=="" then
                if in_headers then
                    in_headers=false
                end
                had_blank=true
            elseif in_headers and not had_status then
                local st, en, stat=string.find(l, '^Status:(.*)')
                if stat then
                    had_status=true
                    if string.find(l, 'R') then
                        read=read+1
                    end
                    if string.find(l, 'O') then
                        old=old+1
                    end
                end
            end
        end
    end
    
    f:close()
    
    return total, total-read, total-old
end

local mail_last_check
local mail_new, mail_unread, mail_total=0, 0, 0

local function update_mail()
    local t=os.time()
    if mail_last_check and t<(mail_last_check+settings.mail_interval) then
        return
    end
    mail_last_check=t

    local mbox=settings.mailbox
    if not mbox or mbox=='' then
        mbox=os.getenv('MAIL')
        if not mbox then
            return
        end
    end
    
    mail_total, mail_unread, mail_new=calcmail(mbox)
end

local function get_mail_new()
    update_mail()
    return tostring(mail_new)
end

local function get_mail_unread()
    update_mail()
    return tostring(mail_unread)
end

local function get_mail_total()
    update_mail()
    return tostring(mail_total)
end

ext_statusbar.register_meter('mail_new', get_mail_new, 'xx')
ext_statusbar.register_meter('mail_unread', get_mail_unread, 'xx')
ext_statusbar.register_meter('mail_total', get_mail_total, 'xx')

-- }}}


-- Status update {{{

function ext_statusbar.get_status()
    return string.gsub(settings.template, '%%([a-zA-Z0-9_]*)', 
                       function(s)
                           if s=="" then
                               return ""
                           end
                           local m=meters[s]
                           if not m then
                               return '??'
                           else
                               return m.fn()
                           end
                       end)
end

function ext_statusbar.get_w_template()
    return string.gsub(settings.template, '%%([a-zA-Z0-9_]*)', 
                       function(s)
                           if s=="" then
                               return ""
                           end
                           local m=meters[s]
                           if not m then
                               return '??'
                           elseif m.width_template then
                               return m.width_template
                           else
                               return m.fn()
                           end
                       end)
end

function ext_statusbar.set_timer()
    local t=os.date('*t')
    local s=((math.floor(t.sec/settings.update_interval)+1)
             *settings.update_interval)
    local d=(s-t.sec)
    if d<=0 then
        d=settings.update_interval
    end
    
    timer:set(d*1000, ext_statusbar.timer_handler)
end

function ext_statusbar.timer_handler(tmr)
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
    
    if found then
        ext_statusbar.set_timer()
    end
end

-- }}}


-- Initialisation {{{

--DOC
-- Create a statusbar.
function ext_statusbar.create(param)
    local scr=ioncore.find_screen_id(param.screen or 0)
    if not scr then
        error("Screen not found")
    end
    
    if not param.force then
        local stdisp=scr:get_stdisp()
        if stdisp and stdisp.reg then
            error("Screen already has an stdisp. Refusing to create a "..
                  "statusbar.")
        end
    end
    
    local iw=scr:set_stdisp({
        type="WInfoWin", 
        corner=(param.corner or "bl"),
        name="*statusbar*",
        style="stdisp-statusbar"
    })
    
    if not iw then
        error("Failed to create statusbar.")
    end
    
    infowins[iw]=true
    iw:set_natural_w(ext_statusbar.get_w_template())
    iw:set_text(ext_statusbar.get_status())
    
    if not timer then
        timer=ioncore.create_timer()
        if not timer then
            error('Failed to create a timer for statusbar.')
        end
    end
    
    if not timer:is_set() then
        ext_statusbar.set_timer()
    end
    
    return iw
end

-- }}}


-- Mark ourselves loaded.
_LOADED["mod_menu"]=true


-- Load user configuration file
dopath('cfg_statusbar', true)

