-- statusd_moc.lua
--
-- Public domain
--
-- keys:
--      moc             (moc_title + "[paused]", "[stopped]", or "[not playing]")
--      moc_state       ("PAUSE", "PLAY", "STOP", or "OFF")
--      moc_state2      ("paused", "playing", "stopped", or "off")
--      moc_file                }
--      moc_title               }
--      moc_artist              }
--      moc_songtitle           }
--      moc_album               }
--      moc_totaltime           }-- (as in "mocp -i")
--      moc_currenttime         }
--      moc_timeleft            }
--      moc_totalsec            }
--      moc_currentsec          }
--      moc_bitrate             }
--      moc_rate                }
--      moc_leftsec     (moc_totalsec - moc_currentsec)
--      moc_percent     (current time / total time)
--      moc_progress    (a progress bar, made of characters---see settings below)
--
-- settings:
--      update_interval
--      moc_dir         (relative to ion's working directory)
--      progress_char1  (character for unfilled progress bar area)
--      progress_char2  (character for filled progress bar area)
--      progress_length (length of the progress bar in characters)
--      color_hints     (boolean-- use color hints for the "state" montitors)

local home=os.getenv("HOME")
home=(home and home.."/" or "")

local defaults={
        update_interval=3*1000,
        moc_dir=home..".moc",
        progress_char1=" ",
        progress_char2="|",
        progress_length="50",
        color_hints=false,
}

local settings=table.join(statusd.get_config("moc"), defaults)

function do_query_mocp()
        local f=io.popen('mocp -i')
        local moc_status = f:read('*a')
        f:close()

        local i, j, moc_state           = string.find( moc_status, 'State: (%C*)')
        local i, j, moc_file            = string.find( moc_status, 'File: (%C*)')
        local i, j, moc_title           = string.find( moc_status, 'Title: (%C*)')
        local i, j, moc_artist          = string.find( moc_status, 'Artist: (%C*)')
        local i, j, moc_songtitle       = string.find( moc_status, 'SongTitle: (%C*)')
        local i, j, moc_album           = string.find( moc_status, 'Album: (%C*)')
        local i, j, moc_totaltime       = string.find( moc_status, 'TotalTime: (%C*)')
        local i, j, moc_currenttime     = string.find( moc_status, 'CurrentTime: (%C*)')
        local i, j, moc_timeleft        = string.find( moc_status, 'TimeLeft: (%C*)')
        local i, j, moc_totalsec        = string.find( moc_status, 'TotalSec: (%C*)')
        local i, j, moc_currentsec      = string.find( moc_status, 'CurrentSec: (%C*)')
        local i, j, moc_bitrate         = string.find( moc_status, 'Bitrate: (%C*)')
        local i, j, moc_rate            = string.find( moc_status, 'Rate: (%C*)')

        if moc_state == "STOP"
        then return "[stopped]", moc_state, "stopped", mocp_not_playing()
        else

        local moc_leftsect, moc_percent, moc_progress = "--", "--", "--"
        if moc_totalsec and moc_currentsec then 
	    moc_leftsec = tostring(moc_totalsec - moc_currentsec)
	    moc_percent = tostring(math.floor(moc_currentsec * 100 / moc_totalsec))
            moc_progress = moc_make_progress_bar(moc_percent)
        end
        
        if moc_state == "PLAY"
        then return moc_title, moc_state, "playing", moc_file, moc_title, moc_artist, moc_songtitle, moc_album, moc_totaltime, moc_currenttime, moc_timeleft, moc_totalsec, moc_currentsec, moc_bitrate, moc_rate, moc_leftsec, moc_percent, moc_progress
        else return moc_title.." [paused]" , moc_state, "paused", moc_file, moc_title, moc_artist, moc_songtitle, moc_album, moc_totaltime, moc_currenttime, moc_timeleft, moc_totalsec, moc_currentsec, moc_bitrate, moc_rate, moc_leftsec, moc_percent, moc_progress
        end
        end
end

function moc_make_progress_bar(p)
        local total_chars = settings.progress_length
        local filled_chars = math.ceil(p * total_chars / 100)
        local str = ""
        for i=1,filled_chars do
                str = str..settings.progress_char2
        end
        for i=filled_chars+1,total_chars do
                str = str..settings.progress_char1
        end
        return str
end

function mocp_not_playing()
        return "--", "--", "--", "--", "--", "--:--", "--:--", "--:--", "--", "--", "--", "--", "--", "--", moc_make_progress_bar(0)
end

function get_moc()
        local f=io.open(settings.moc_dir.."/pid")
        if f
        then    f:close()
                return do_query_mocp()
        else    return "[not running]", "OFF", "off", mocp_not_playing()
        end
end

function update_moc()
        local moc, moc_state, moc_state2, moc_file, moc_title, moc_artist, moc_songtitle, moc_album, moc_totaltime, moc_currenttime, moc_timeleft, moc_totalsec, moc_currentsec, moc_bitrate, moc_rate, moc_leftsec, moc_percent, moc_progress = get_moc()
	statusd.inform("moc", moc)
        statusd.inform("moc_state", moc_state)
        statusd.inform("moc_state2", moc_state2)
        if settings.color_hints then
        if moc_state2 == "playing"
        then    statusd.inform("moc_state_hint", "green")
                statusd.inform("moc_state2_hint", "green")
        elseif moc_state2 == "paused"
        then    statusd.inform("moc_state_hint", "yellow")
                statusd.inform("moc_state2_hint", "yellow")
        elseif moc_state2 == "stopped"
        then    statusd.inform("moc_state_hint", "red")
                statusd.inform("moc_state2_hint", "red")
        elseif moc_state2 == "off"
        then    statusd.inform("moc_state_hint", "grey")
                statusd.inform("moc_state2_hint", "grey")
        end end
        statusd.inform("moc_file", moc_file)
        statusd.inform("moc_title", moc_title)
        statusd.inform("moc_artist", moc_artist)
        statusd.inform("moc_songtitle", moc_songtitle)
        statusd.inform("moc_album", moc_album)
        statusd.inform("moc_totaltime", moc_totaltime)
        statusd.inform("moc_currenttime", moc_currenttime)
        statusd.inform("moc_timeleft", moc_timeleft)
        statusd.inform("moc_totalsec", moc_totalsec)
        statusd.inform("moc_currentsec", moc_currentsec)
        statusd.inform("moc_bitrate", moc_bitrate)
        statusd.inform("moc_rate", moc_rate)
        statusd.inform("moc_leftsec", moc_leftsec)
        statusd.inform("moc_percent", moc_percent)
        statusd.inform("moc_progress", moc_progress)
	moc_timer:set(settings.update_interval, update_moc)
end

moc_timer = statusd.create_timer()
update_moc()

-- vim:tw=0:
