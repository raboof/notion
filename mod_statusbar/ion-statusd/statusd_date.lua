--
-- ion/mod_statusbar/ion-statusd/statusd_date.lua
-- 
-- Copyright (c) Tuomo Valkonen 2004-2005.
--
-- Ion is free software; you can redistribute it and/or modify it under
-- the terms of the GNU Lesser General Public License as published by
-- the Free Software Foundation; either version 2.1 of the License, or
-- (at your option) any later version.
--


local timer

local defaults={
    date_format='%a %Y-%m-%d %H:%M',
    formats={},
}

local settings=table.join(statusd.get_config('date'), defaults)

local function update()
    local tm=os.time()
    statusd.inform('date', os.date(settings.date_format, tm))
    for k, f in settings.formats do
        statusd.inform('date_'..k, os.date(f, tm))
    end
    return tm
end

local function timer_handler(tmr)
    local tm=update()
    
    local t=os.date('*t', tm)
    local d=(60-t.sec)*1000
    
    timer:set(d, timer_handler)
end

timer=statusd.create_timer()
timer_handler(timer)
