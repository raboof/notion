--
-- Ion statusbar extension configuration file
-- 


-- Configure format and updating
ext_statusbar.set{
    -- ion-statusd params. Load mail checker and load modules.
    -- (The load meter should really be on the Ion side for up-to-datedness, 
    -- but until ext_statusbar is turned into a proper C module that can use 
    -- getloadavg, the code uses potentially blocking files and external 
    -- programs and thus belongs to ion-statusd.)
    --statusd_params="-m mail -m load"

    -- ISO-8601 date format with additional abbreviated day name
    --date_format='%a %Y-%m-%d %H:%M',
    -- Finnish date format
    --date_format='%a %d.%m.%Y %H:%M'
    -- Locale date format (usually shows seconds, which would require
    -- updating rather often and can be distracting)
    --date_format='%c',

    -- Template. Tokens %string are replaced with the value of the 
    -- corresponding meter. Currently supported meters are:
    --   %date     date
    --   %load     load average
    --   %mail_new    new mail count
    --   %mail_unread unread mail count
    --   %mail_total  total mail count
    --template="[ %date || load: %load || mail: %mail_new/%mail_total ]",
}


-- Create a statusbar
ext_statusbar.create{
    -- First screen, bottom left corner
    screen=0,
    pos='bl',
}
