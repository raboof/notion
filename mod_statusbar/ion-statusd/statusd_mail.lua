--
-- ion/ext_statusbar/ion-statusd/statusd_mail.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2005.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local defaults={
    update_interval=10*1000,
    retry_interval=60*10*1000,
    mbox=os.getenv("MAIL")
}

local function TR(s, ...)
    return string.format(statusd.gettext(s), unpack(arg))
end

local settings=table.join(statusd.get_config("mail"), defaults)

local function calcmail(fname)
    local f, err=io.open(fname, 'r')
    local total, read, old=0, 0, 0
    local had_blank=true
    local in_headers=false
    local had_status=false
    
    if not f then
        statusd.warn(err)
        return
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

local mail_timer
local mail_timestamp=-2.0

local function update_mail()
    assert(settings.mbox)
    
    local old_tm=mail_timestamp
    mail_timestamp=statusd.last_modified(settings.mbox)
    
    if mail_timestamp>old_tm then
        local mail_total, mail_unread, mail_new=calcmail(settings.mbox)
        
        if not mail_total then
            statusd.warn(TR("Disabling mail monitor for %d seconds.",
                            settings.retry_interval/1000))
            mail_timestamp=-2.0
            mail_timer:set(settings.retry_interval, update_mail)
            return
        end
        
        statusd.inform("mail_new", tostring(mail_new))
        statusd.inform("mail_unread", tostring(mail_unread))
        statusd.inform("mail_total", tostring(mail_total))
        
        if mail_new>0 then
            statusd.inform("mail_new_hint", "important")
        else
            statusd.inform("mail_new_hint", "normal")
        end
        
        if mail_unread>0 then
            statusd.inform("mail_unread_hint", "important")
        else
            statusd.inform("mail_unread_hint", "normal")
        end
    end

    mail_timer:set(settings.update_interval, update_mail)
end

-- Init
statusd.inform("mail_new_template", "00")
statusd.inform("mail_unread_template", "00")
statusd.inform("mail_total_template", "00")

mail_timer=statusd.create_timer()
update_mail()

