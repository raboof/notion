--
-- Ion statusbar extension configuration file
-- 


-- Configure format and updating
ext_statusbar.set{
    -- ISO-8601 date format
    date_format='%Y-%m-%d %H:%M',
    -- Finnish date format
    --date_format='%d.%m.%Y %H:%M'
    -- Locale date format (usually shows seconds, which would require
    -- updating rather often and can be distracting)
    --date_format='%c',

    -- Update interval in seconds. This should divide 60, or it will
    -- get adjusted.
    update_interval=10,
    
    -- Mail checking interval
    mail_interval=60,
    
    -- Template. Tokens %string are replaced with the value of the 
    -- corresponding meter. Currently supported meters are:
    --   %date     date
    --   %load     load average
    --   %mail_new    new mail count
    --   %mail_unread unread mail count
    --   %mail_total  total mail count
    template="%date || load: %load || mail: %mail_new/%mail_total",
}


-- Create a statusbar
ext_statusbar.create{
    -- First screen, bottom left corner
    screen=0,
    corner='bl',
}
