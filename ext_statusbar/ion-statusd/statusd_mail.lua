--
-- ion/ext_statusbar/ion-statusd/statusd_mail.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

local settings={
    interval=60*1000,
    mbox=os.getenv("MAIL")
}

local function calcmail(fname)
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

local function update_mail(timer)
    assert(settings.mbox)
    
    local mail_total, mail_unread, mail_new=calcmail(settings.mbox)
    
    statusd.inform("mail_new", tostring(mail_new))
    statusd.inform("mail_unread", tostring(mail_unread))
    statusd.inform("mail_total", tostring(mail_total))

    timer:set(settings.interval, update_mail)
end

-- Init
local timer=statusd.create_timer()
update_mail(timer)
