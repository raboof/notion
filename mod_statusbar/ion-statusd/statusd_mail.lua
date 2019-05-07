--
-- ion/mod_statusbar/ion-statusd/statusd_mail.lua
--
-- Copyright (c) Tuomo Valkonen 2004-2006.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--

-- The keyword for this monitor
local mon = "mail"

local defaults={
    update_interval=10*1000,
    retry_interval=60*10*1000,
    mbox = os.getenv("MAIL"),
    files = {}
}

local settings=table.join(statusd.get_config(mon), defaults)

local function TR(s, ...)
    return string.format(statusd.gettext(s), unpack(arg))
end

local function check_spool()
    if not settings.mbox then
        statusd.warn(TR("MAIL environment variable not set "..
                        "and no spool given."))
    end
end

if not settings.files["spool"] then
    check_spool()
    settings.files["spool"] = settings.mbox
elseif not(settings.files["spool"] == mbox) then
    statusd.warn(TR("%s.mbox does not match %s.files['spool']; using %s.mbox",
		    mon, mon, mon))
    check_spool()
    settings.files["spool"] = settings.mbox
end

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
local mail_timestamps = {}
function init_timestamps ()
    for key, val in pairs(settings.files) do
        mail_timestamps[key]=-2.0
    end
end
init_timestamps()

local function update_mail()
    local failed
    for key, mbox in pairs(settings.files) do
        if not mbox then
            error(TR(key.." not set"))
        end

        local old_tm=mail_timestamps[key]
        mail_timestamps[key]=statusd.last_modified(mbox)

        if mail_timestamps[key]>old_tm then
	    local mail_total, mail_unread, mail_new=calcmail(mbox)
	    if failed == nil then
	        failed = not mail_total
	    else
	        failed = failed and (not mail_total)
	    end

	    if key == "spool" then
	        meter=mon
	    else
	        meter=mon.."_"..key
	    end
	    if mail_total then
	        statusd.inform(meter.."_new", tostring(mail_new))
	        statusd.inform(meter.."_unread", tostring(mail_unread))
	        statusd.inform(meter.."_total", tostring(mail_total))

	        if mail_new>0 then
		    statusd.inform(meter.."_new_hint", "important")
	        else
		    statusd.inform(meter.."_new_hint", "normal")
	        end

	        if mail_unread>0 then
		    statusd.inform(meter.."_unread_hint", "important")
	        else
		    statusd.inform(meter.."_unread_hint", "normal")
	        end
	    end
	end
    end

    if failed then
        statusd.warn(TR("Disabling mail monitor for %d seconds.",
			settings.retry_interval/1000))
	init_timestamps()
	mail_timer:set(settings.retry_interval, update_mail)
	return
    end

    mail_timer:set(settings.update_interval, update_mail)
end

mail_timer=statusd.create_timer()
update_mail()

