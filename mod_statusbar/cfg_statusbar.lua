--
-- Ion statusbar module configuration file
-- 


-- Create a statusbar
mod_statusbar.create{
    -- First screen, bottom left corner
    screen=0,
    pos='bl',

    -- ISO-8601 date format with additional abbreviated day name
    date_format='%a %Y-%m-%d %H:%M',
    -- Finnish etc. date format
    --date_format='%a %d.%m.%Y %H:%M'
    -- Locale date format (usually shows seconds, which would require
    -- updating rather often and can be distracting)
    --date_format='%c',

    -- Template. Tokens %string are replaced with the value of the 
    -- corresponding meter. Currently supported meters are:
    --   date              date
    --   load              load average
    --   mail_spool_new    new mail count (mbox format file $MAIL)
    --   mail_spool_unread unread mail count
    --   mail_spool_total  total mail count
    -- Space preceded by % adds stretchable space. > before meter name 
    -- aligns right, < left, and | centers.
    template="[ %date || load:% %>load || mail:% %>mail_spool_new/%>mail_spool_total ]",
    --template="[ %date || load: %load || mail: %mail_spool_new/%mail_spool_total ]",
}


-- Launch ion-statusd. This must be done after creating any statusbars
-- for necessary modules to be parsed from the templates.
mod_statusbar.launch_statusd{
    -- Load meter
    --[[
    load={
        update_interval=10*1000,
        important_threshold=1.5,
        critical_threshold=4.0
    },
    --]]

    -- Mail meter
    --[[
    mail={
        update_interval=60*1000,
        mbox={spool = os.getenv("MAIL")}
    },
    --]]
}

